
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
    
    std::string test = "1.31a";
    std::cout << isSanitized( &test, "01234567890." ) << std::endl;
    
    return 0;
}
