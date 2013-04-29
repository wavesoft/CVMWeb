
#include <boost/thread.hpp>

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Hypervisor.h"
#include "Virtualbox.h"
#include "ThinIPC.h"

using namespace std;

int main( int argc, char ** argv ) {
    
    // Validate cmdline
    if (argc < 3) {
        cerr << "ERROR: Use syntax: daemon \"<VM UUID>\" \"<IPC Port>\"\n";
        return 1;
    }
    
    // Get a hypervisor control instance
    Hypervisor *hv = detectHypervisor();
    if (hv == NULL) {
        cerr << "ERROR: Unable to detect hypervisor!\n";
        return 2;
    }
    
    // This is working only with VirtualBox
    if (hv->type != HV_VIRTUALBOX) {
        cerr << "ERROR: Only VirtualBox is currently supproted!\n";
        return 3;
    }
    
    // Open the specified session
    HVSession * sess = hv->sessionLocate( argv[1] );
    if (sess == NULL) {
        cerr << "ERROR: Unable to find the specified session!\n";
        return 2;
    }
    
    // Start VM
    string cmdline = hv->hvRoot;
    cout << "Launching ... \n";
    cmdline += "/VBoxHeadless -s \"";
    cmdline += argv[1];
    cmdline += "\"";
    system( cmdline.c_str() );
    
    return 0;
}
