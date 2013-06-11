
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Utilities.h"
#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"

#include <openssl/rand.h>

void logUpdates(int curr, int tot, std::string msg, void * o) {
    std::cout << "(" << curr << "/" << tot << " %) " << msg << std::endl;
};

void logDebug( std::string line, void * o ) {
    std::cout << "DEBUG: " << line << std::endl;
};

std::string generateSalt() {
    const unsigned char SALT_SIZE = 64;
    const char saltChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+";
    unsigned char randValue[SALT_SIZE];
    std::string saltData = "";
    
    // Generate random bytes
    RAND_bytes( randValue, SALT_SIZE );
    
    // Map random value to string
    for (int i=0; i<SALT_SIZE; i++) {
        saltData += saltChars[ randValue[i] % 0x3F ];
    }
    return saltData;
}

int main( int argc, char ** argv ) {
    
    std::string data;
    for (int i=0; i<1000; i++) {
        downloadTextEx("http://localhost/", &data, NULL, NULL);
        std::cout << data << std::endl;
        usleep( 100000 );
    }
    
    return 0;
}
