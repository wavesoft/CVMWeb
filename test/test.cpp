
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"

void logUpdates(int curr, int tot, std::string msg, void * o) {
    std::cout << "(" << curr << "/" << tot << " %) " << msg << "\n";
};

int main( int argc, char ** argv ) {
    Hypervisor * hv;
    hv = detectHypervisor();
        
    if (hv != NULL) {
        std::string data;
        hv->buildContextISO( "Here are some user data", &data );
        std::cout << "Your file is at " << data << std::endl;
    }
}

int zmain( int argc, char ** argv ) {
    ThinIPCMessage   msg;
    char buf[512];
    
    installHypervisor( "1.0", logUpdates, NULL );
    return 0;
    
    // Initialize IPC
    ThinIPCInitialize(); 
    
    /*
    msg.writeString("Hello-world");
    msg.writeString("I am god");
    msg.writeInt(41412);
    msg.writeInt(512);
    
    ThinIPCEndpoint ep(51301);
    int dlen = ep.send( 51302, &msg );
    std::cout << "Return = " << dlen << "\n";
    */
    
    ThinIPCEndpoint ep(51302);
    int dlen = ep.recv( 51301, &msg );

    std::cout << "Return = " << dlen << "\n";
    std::cout << "String: " << msg.readString() << "\n";
    std::cout << "String: " << msg.readString() << "\n";
    std::cout << "Int: " << msg.readInt() << "\n";
    std::cout << "Int: " << msg.readInt() << "\n";

}

int none() {
    Hypervisor * hv;
    hv = detectHypervisor();
        
    if (hv != NULL) {
        printf("Hypervisor Type     = %i\n", hv->type);
        printf("Hypervisor Root     = %s\n", hv->hvRoot.c_str());
        printf("Hypervisor Binary   = %s\n", hv->hvBinary.c_str());
        printf("Hypervisor Version  = %s (%i.%i)\n", hv->verString.c_str(), hv->verMajor, hv->verMinor);
        
        HVINFO_CAPS caps;
        hv->getCapabilities( &caps );
        std::cout << "CPU: " << caps.cpu.vendor << "(" << (int)caps.cpu.family << "/" << (int)caps.cpu.model << "." << (int)caps.cpu.stepping << ")\n";
        std::cout << "VT-x: " << ( caps.cpu.hasVT ? "yes" : "no" ) << "\n";
        std::cout << "64-bit: " << ( caps.cpu.has64bit ? "yes" : "no" ) << "\n";
        std::cout << "Max CPU: " << caps.max.cpus << "\n";
        std::cout << "Max RAM: " << caps.max.memory << "\n";
        std::cout << "Max HDD: " << caps.max.disk << "\n";
        
        
        if (hv->type == 1) {
            printf("Guest Additions     = %s\n", ((Virtualbox *)hv)->hvGuestAdditions.c_str());
            /*
            HVSession * sess = hv->sessionOpen("johnathan", "eaklfjq093j1");
            std::cout << "Got session name=" << sess->name << ", key=" << sess->key << ", uuid=" << ((VBoxSession*)sess)->uuid << ", state=" << sess->state << "\n";
            sess->open(1, 256, 1024, "1.3");
            sess->close();
            */
            //sess->start("[cernvm]\nusers=user:users:s3cr3t\n");
            /*
            std::string f;
            hv->downloadCernVM("1.3", &f);
            std::cout << "Got file " << f << "\n";
            */
        }
        
    } else {
        printf("No hypervisor found!\n");
    }
    
    installHypervisor( "1.0", NULL, NULL );

    return 0;
};