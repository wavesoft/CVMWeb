
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Utilities.h"
#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"
#include "CVMWebCrypto.h"

void logUpdates(int curr, int tot, std::string msg, void * o) {
    std::cout << "(" << curr << "/" << tot << " %) " << msg << std::endl;
};

void logDebug( std::string line, void * o ) {
    std::cout << "DEBUG: " << line << std::endl;
};

int main( int argc, char ** argv ) {
    
    std::cout << getFilename( "/Users/icharala/Library/Application Support/CernVM/WebAPI/cache/disk-5a6df716cd360c13f6ec7cfff3314b218462f3e627027a0d1a57d2d9ff7aab2b.vdi" ) << std::endl;
    
    return 0;
}
