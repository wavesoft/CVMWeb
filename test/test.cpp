
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Utilities.h"
#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"

#include "DownloadProvider.h"

#include <openssl/rand.h>

void logUpdates(int curr, int tot, std::string msg, void * o) {
    std::cout << "(" << curr << "/" << tot << " %) " << msg << std::endl;
};

void logDebug( std::string line, void * o ) {
    std::cout << "DEBUG: " << line << std::endl;
};

void cb( const long a, const long f, const std::string& m ) {
    std::cout << "*** CB " << a << "/" << f << " : " << m << " ***" << std::endl; 
}

int main( int argc, char ** argv ) {
    
    decompressFile(
            "/Users/icharala/Downloads/cernvm-basic-2.7.1-2-2-x86.vdi.gz",
            "/Users/icharala/Downloads/cernvm-basic-2.7.1-2-2-x86.vdi"
        );
    
    return 0;
}
