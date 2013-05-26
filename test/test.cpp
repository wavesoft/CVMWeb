
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include <libproc.h>

#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"

void logUpdates(int curr, int tot, std::string msg, void * o) {
    std::cout << "(" << curr << "/" << tot << " %) " << msg << std::endl;
};

void logDebug( std::string line, void * o ) {
    std::cout << "DEBUG: " << line << std::endl;
};

int main( int argc, char ** argv ) {

    LINUX_INFO info;
    getLinuxInfo( &info );
    
    std::cout << "Package Manager = " << (int) info.osPackageManager << std::endl;
    std::cout << "Has GKSudo = " << info.hasGKSudo << std::endl;
    std::cout << "Platform ID = " << info.osDistID << std::endl;
    
    return 0;
}

int no_main( int argc, char ** argv ) {
    
    // Initialize IPC
    ThinIPCInitialize(); 
    
    // Query idle time
    int idleTime = daemonGet( DIPC_GET_IDLETIME );
    if (idleTime < 0) {
        std::cout << "ERROR: Could not query daemon (" << idleTime << ")" << std::endl;
    }
    
    std::cout << "idleTime=" << idleTime << std::endl;
    
    // Update idle time
    int res = daemonSet( DIPC_SET_IDLETIME, idleTime + 10 );
    if (res < 0) {
        std::cout << "ERROR: Could not update daemon (" << idleTime << ")" << std::endl;
    }

    // Query idle time
    idleTime = daemonGet( DIPC_GET_IDLETIME );
    if (idleTime < 0) {
        std::cout << "ERROR: Could not query daemon (" << idleTime << ")" << std::endl;
    }
    
    std::cout << "idleTime=" << idleTime << std::endl;
    
    // Update idle time
    res = daemonSet( DIPC_SET_IDLETIME, idleTime - 10 );
    if (res < 0) {
        std::cout << "ERROR: Could not update daemon (" << idleTime << ")" << std::endl;
    }

    // Query idle time
    idleTime = daemonGet( DIPC_GET_IDLETIME );
    if (idleTime < 0) {
        std::cout << "ERROR: Could not query daemon (" << idleTime << ")" << std::endl;
    }
    
    std::cout << "idleTime=" << idleTime << std::endl;
    
    // Update idle time
    res = daemonSet( DIPC_SET_IDLETIME, idleTime + 10 );
    if (res < 0) {
        std::cout << "ERROR: Could not update daemon (" << idleTime << ")" << std::endl;
    }

    // Query idle time
    idleTime = daemonGet( DIPC_GET_IDLETIME );
    if (idleTime < 0) {
        std::cout << "ERROR: Could not query daemon (" << idleTime << ")" << std::endl;
    }
    
    std::cout << "idleTime=" << idleTime << std::endl;
    
    // Update idle time
    res = daemonSet( DIPC_SET_IDLETIME, idleTime - 10 );
    if (res < 0) {
        std::cout << "ERROR: Could not update daemon (" << idleTime << ")" << std::endl;
    }
            
    return 0;

}