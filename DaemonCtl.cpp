
#include "DaemonCtl.h"
#include "Utilities.h"

#include <sstream>
#include <stdlib.h>

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
std::string getDaemonLockfile ( std::string exePath ) {
    /* Return path + '.pid'  */
    return exePath + ".pid";
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
        return (ret != 0);
    #endif
    #ifdef _WIN32
        LPDWORD lpExitCode;
        GetExitCodeProcess( pid, &lpExitCode );
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
        int pid = getpid();
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
int daemonIPC( ThinIPCMessage * send, ThinIPCMessage * recv ) {
    
    /* Pick a random port */
    static int rPort = (random() % ( 0xFFFE - DAEMON_PORT )) + DAEMON_PORT + 1;
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
    int errNo = recv->readInt();
    if (errNo != DIPC_ANS_OK)
        return -abs(errNo);
    
    /* Success */
    return 0;
    
}

/**
 * Contact daemon requesting integer response
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
long int daemonGet( long int action ) {
    static ThinIPCMessage send, recv;
    send.reset(); recv.reset();
    send.writeInt( action );
    int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readInt();
}

/**
 * Contact daemon setting integer data
 * (Shorthand for calling daemonIPC with ThinIPCMessage structures)
 */
long int daemonSet( long int action, long int value ) {
    static ThinIPCMessage send, recv;
    send.reset();
    recv.reset();
    send.writeInt( action );
    send.writeInt( value );
    int res = daemonIPC( &send, &recv );
    if (res != 0) return res;
    return recv.readInt();
}