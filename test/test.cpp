
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

class miClass {
public:
    
    miClass() {
        std::cout << "CONSTRUCTED!" << std::endl;
    }

    ~miClass() {
        std::cout << "DESTRUCTED!" << std::endl;
    }
    
};

typedef struct T {
    
    miClass * p;
    
};

int main( int argc, char ** argv ) {
    
    std::cout << "Allocating T" << std::endl;
    T * t = new T;
    std::cout << "Allocating miClass" << std::endl;
    t->p = new miClass();
    std::cout << "Destructing T" << std::endl;
    delete t->p;
    delete t;
    
    std::cout << "About to returm" << std::endl;
    return 0;
}
