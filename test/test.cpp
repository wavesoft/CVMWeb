
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Virtualbox.h"
#include "Hypervisor.h"

void logUpdates(std::string msg, int progress, void * o) {
    std::cout << "(" << progress << " %) " << msg << "\n";
};

int main( int argc, char ** argv ) {

    Hypervisor * hv;
    hv = detectHypervisor();
    
    if (hv != NULL) {
        printf("Hypervisor Type     = %i\n", hv->type);
        printf("Hypervisor Root     = %s\n", hv->hvRoot.c_str());
        printf("Hypervisor Binary   = %s\n", hv->hvBinary.c_str());
        printf("Hypervisor Version  = %s (%i.%i)\n", hv->verString.c_str(), hv->verMajor, hv->verMinor);
        
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