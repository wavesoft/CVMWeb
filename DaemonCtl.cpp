
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
#include <signal.h>
#endif

using namespace std;

/**
 * Return the location of the daemon lockfile
 */
std::string getDaemonLockfile() {
    /* Return the system-wide daemon pidfile location  */
    std::string appDir = getAppDataPath();
    return appDir + "/run/daemon.pid";
}

/**
 * Check if the given process ID is still running
 */
bool isAlive( int pid ) {
    
    #if defined(__APPLE__) && defined(__MACH__)
        struct proc_bsdinfo bsdInfo;
        int ret = proc_pidinfo( pid, PROC_PIDTBSDINFO, 0, &bsdInfo, sizeof(bsdInfo));
        return (ret > 0);
    #endif
    #ifdef __linux__
        int ret = kill( pid, 0 );
        return (ret == 0);
    #endif
    #ifdef _WIN32
        DWORD lpExitCode;
        
        // Open handle
        HANDLE hProc = OpenProcess( PROCESS_QUERY_INFORMATION, false, pid);
        if (hProc == NULL) return false;
        
        // Query exit code (returns STILL_ALIVE if it's still alive)
        GetExitCodeProcess( hProc, &lpExitCode );
        CloseHandle( hProc );

        // Check status
        return (lpExitCode == STILL_ACTIVE);
    #endif

    return false;
    
}

/**
 * Check if the file exists and is locked
 */
bool isDaemonRunning ( std::string lockfile ) {
    
    /* Lockfile not there, it's not running */
    if (!file_exists(lockfile))
        return false;
    
    /* Check if we can acquire lock */
    ifstream lfStream;
    lfStream.open( lockfile.c_str(), std::ios_base::in );
    if (!lfStream.fail()) {
        
        /* Get PID */
        int pid; lfStream >> pid;
        lfStream.close();

        /* Check if it's running */
        return isAlive( pid );
        
    }
    
    /* Not running */
    return false;
    
}

/**
 * Try to lock or unlock the file
 */
DLOCKINFO * daemonLock( std::string lockfile ) {
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
    
}

/**
 * Unlock the daemon lockfile
 */
void daemonUnlock( DLOCKINFO * dlInfo ) {
    /* Remove lockfile */
    remove( dlInfo->fname.c_str() );
}

/**
 * Contact daemon
 * WARNING! ThinIPCInitialize() *MUST* be called in advance!
 */
short int daemonIPC( ThinIPCMessage * send, ThinIPCMessage * recv ) {
    
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
    
}

/**
 * Contact daemon requesting integer response
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
short int daemonGet( short int action ) {
    static ThinIPCMessage send, recv;
    send.reset(); recv.reset();
    send.writeShort( action );
    short int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readShort();
}

/**
 * Contact daemon setting integer data
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
short int daemonSet( short int action, short int value ) {
    static ThinIPCMessage send, recv;
    send.reset();
    recv.reset();
    send.writeShort( action );
    send.writeShort( value );
    int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readShort();
}

/**
 * Cross-platform way to start the daemon process in the background
 */
int daemonStart( std::string path_to_bin ) {
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
        int pid = fork();
        if (pid==0) {
            setsid();
            execl( path_to_bin.c_str(), path_to_bin.c_str(), (char*) 0 );
            return 0;
        } else {
            return HVE_SCHEDULED;
        }
    #endif
    
};