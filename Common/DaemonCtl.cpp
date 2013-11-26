/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

#include "DaemonCtl.h"
#include "Utilities.h"
#include "Hypervisor.h"

#include <sstream>
#include <cstdlib>

#if defined(__APPLE__) && defined(__MACH__)
#include <libproc.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

using namespace std;

/**
 * Current daemon pid
 */
int currDaemonPid = 0;

/**
 * Return the location of the daemon lockfile
 */
std::string getDaemonLockfile() {
    CRASH_REPORT_BEGIN;
    /* Return the system-wide daemon pidfile location  */
    std::string appDir = getAppDataPath();
    return appDir + "/run/daemon.pid";
    CRASH_REPORT_END;
}

/**
 * Return the daemon pid from the lockfile
 */
int daemonLockFilePID () {
    CRASH_REPORT_BEGIN;
    std::string lockfile = getDaemonLockfile();
    
    /* If thre is no lockfile return 0 */
    if (!file_exists(lockfile))
        return 0;
    
    /* Read file */
    ifstream lfStream;
    lfStream.open( lockfile.c_str(), std::ios_base::in );
    if (!lfStream.fail()) {
        /* Get PID */
        int pid; lfStream >> pid;
        lfStream.close();
        return pid;
    }
    
    /* Unable to read file */
    return 0;
    CRASH_REPORT_END;
}

/**
 * Check if the file exists and is locked
 */
bool isDaemonRunning () {
    CRASH_REPORT_BEGIN;
    
    /* Fetch daemon PID */
    int pid = daemonLockFilePID();
    if (pid == 0)
        return false;
    
    /* Check if it's running */
    return isPIDAlive( pid );
    
    CRASH_REPORT_END;
}

/**
 * Try to lock or unlock the file
 */
DLOCKINFO * daemonLock( std::string lockfile ) {
    CRASH_REPORT_BEGIN;
    static DLOCKINFO dlInfo;
        
    /* Open file for output */
    dlInfo.fname = lockfile;
    ofstream lfStream;
    lfStream.open( lockfile.c_str(), std::ios_base::out );
    if (!lfStream.fail()) {
        
        /* Write pid */
        int pid;
        #ifndef _WIN32
        pid = getpid();
        #else
        pid = GetCurrentProcessId();
        #endif
        lfStream << pid << endl;
        lfStream.close();
        
        return &dlInfo;
        
    } else {
        return NULL;
    }
    
    CRASH_REPORT_END;
}

/**
 * Unlock the daemon lockfile
 */
void daemonUnlock( DLOCKINFO * dlInfo ) {
    CRASH_REPORT_BEGIN;
    /* Remove lockfile */
    remove( dlInfo->fname.c_str() );
    CRASH_REPORT_END;
}

/**
 * Contact daemon
 * WARNING! ThinIPCInitialize() *MUST* be called in advance!
 */
short int daemonIPC( ThinIPCMessage * send, ThinIPCMessage * recv ) {
    CRASH_REPORT_BEGIN;
    
    /* Pick a random port */
    static int rPort = (rand() % ( 0xFFFE - DAEMON_PORT )) + DAEMON_PORT + 1;
    static ThinIPCEndpoint ipc( rPort );
    
    /* Send message */
    if (ipc.send( DAEMON_PORT, send ) < 0)
        return -1;
    
    /* Wait 250 ms for answer */
    if (!ipc.isPending( 250000 ))
        return -2;
    
    /* Read response */
    int port;
    ipc.recv( &port, recv );
    if (port != DAEMON_PORT)
        return -3;
    
    /* Check if we had error */
    int errNo = recv->readShort();
    if (errNo != DIPC_ANS_OK)
        return -abs(errNo);
    
    /* Success */
    return 0;
    
    CRASH_REPORT_END;
}

/**
 * Contact daemon requesting integer response
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
short int daemonGet( short int action ) {
    CRASH_REPORT_BEGIN;
    static ThinIPCMessage send, recv;
    send.reset(); recv.reset();
    send.writeShort( action );
    short int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readShort();
    CRASH_REPORT_END;
}

/**
 * Contact daemon setting integer data
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
short int daemonSet( short int action, short int value ) {
    CRASH_REPORT_BEGIN;
    static ThinIPCMessage send, recv;
    send.reset();
    recv.reset();
    send.writeShort( action );
    send.writeShort( value );
    int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readShort();
    CRASH_REPORT_END;
}

/**
 * Cross-platform way to start the daemon process in the background
 */
int daemonStart( std::string path_to_bin ) {
    CRASH_REPORT_BEGIN;
    #ifdef _WIN32
        HINSTANCE hApp = ShellExecuteA(
            NULL,
            NULL,
            path_to_bin.c_str(),
            NULL,
            NULL,
            SW_HIDE);
        if ( (int)hApp > 32 ) {
            return HVE_SCHEDULED;
        } else {
            return HVE_IO_ERROR;
        }
    #else
        
        // Use a local pointer for the fork
        static char file[4096];
        path_to_bin.copy( file, path_to_bin.length(), 0 );
        file[path_to_bin.length()] = '\0';
        
        // Fork and start
        int pid = fork();
        if (pid==0) {
            setsid();
            CVMWA_LOG("Daemon", "Running " << file );
            execl( file, file, (char*) 0 );
            return 0;
        } else {
            currDaemonPid = pid;
            return HVE_SCHEDULED;
        }
    #endif
    
    CRASH_REPORT_END;
};

/**
 * Shut down and reap daemon
 */
int daemonStop( ) {
    CRASH_REPORT_BEGIN;
    
    /* (Assuming the user did his homework and checked if the daemon was running) */
    int ipcRes = daemonGet( DIPC_SHUTDOWN );
    if (ipcRes < 0) return ipcRes;
    
    #ifndef _WIN32
    /* (*NIX platforms require child reap in order to avoid leaving zombies) */

    /* Try to find the daemon PID to reap */
    int reapPid = currDaemonPid;
    if (reapPid == 0) reapPid = daemonLockFilePID();
    
    /* Wait PID */
    pid_t pid; int status;
    if (reapPid != 0) while((pid = waitpid(reapPid, &status, 0)) > 0) ;
    
    #endif
    
    /* Everything is OK */
    return HVE_OK;
    CRASH_REPORT_END;
}