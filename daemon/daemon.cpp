/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

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
