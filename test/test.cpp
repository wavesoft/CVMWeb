
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

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

    /* Debug platform detection */
    LINUX_INFO info;
    getLinuxInfo( &info );
    std::cout << "Package Manager = " << (int) info.osPackageManager << std::endl;
    std::cout << "Has GKSudo = " << info.hasGKSudo << std::endl;
    std::cout << "Platform ID = " << info.osDistID << std::endl;

    /* Debug installer */
    installHypervisor( "1.0.8", &logUpdates, NULL );
    
    return 0;
}