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

#include "VBoxCommon.h"
#include "VBoxInstance.h"
using namespace std;

/**
 * Allocate a new hypervisor using the specified paths
 */
HVInstancePtr __vboxInstance( string hvRoot, string hvBin, string hvAdditionsIso ) {
    VBoxInstancePtr hv;

    // Create a new hypervisor instance
    hv = boost::make_shared<VBoxInstance>( hvRoot, hvBin, hvAdditionsIso );

    return hv;
}

/**
 * Search Virtualbox on the environment and return an initialized
 * VBoxInstance object if it was found.
 */
HVInstancePtr vboxDetect() {
    HVInstancePtr hv;
    vector<string> paths;
    string bin, iso, p;
    
    // In which directories to look for the binary
    #ifdef _WIN32
    paths.push_back( "C:/Program Files/Oracle/VirtualBox" );
    paths.push_back( "C:/Program Files (x86)/Oracle/VirtualBox" );
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    paths.push_back( "/Applications/VirtualBox.app/Contents/MacOS" );
    #endif
    #ifdef __linux__
    paths.push_back( "/usr" );
    paths.push_back( "/usr/local" );
    paths.push_back( "/opt/VirtualBox" );
    #endif
    
    // Detect hypervisor
    for (vector<string>::iterator i = paths.begin(); i != paths.end(); i++) {
        p = *i;
        
        // Check for virtualbox
        #ifdef _WIN32
        bin = p + "/VBoxManage.exe";
        if (file_exists(bin)) {

            // Check for iso
            iso = p + "/VBoxGuestAdditions.iso";
            if (!file_exists(iso)) iso = "";

            // Instance hypervisor
            hv = __vboxInstance( p, bin, iso );
            break;

        }
        #endif

        #if defined(__APPLE__) && defined(__MACH__)
        bin = p + "/VBoxManage";
        if (file_exists(bin)) {

            // Check for iso
            iso = p + "/VBoxGuestAdditions.iso";
            if (!file_exists(iso)) iso = "";

            // Instance hypervisor
            hv = __vboxInstance( p, bin, iso );
            break;

        }
        #endif

        #ifdef __linux__
        bin = p + "/bin/VBoxManage";
        if (file_exists(bin)) {

            // (1) Check for additions on XXX/share/virtualbox [/usr, /usr/local]
            iso = p + "/share/virtualbox/VBoxGuestAdditions.iso";
            if (!file_exists(iso)) {
                // (2) Check for additions on XXX/additions [/opt/VirtualBox]
                iso = p + "/additions/VBoxGuestAdditions.iso";
                if (!file_exists(iso)) {
                    iso = "";
                }
            }

            // Instance hypervisor
            hv = __vboxInstance( p, bin, iso );

        }
        #endif
        
    }

    // Return hypervisor instance or nothing
	return hv;

}

/**
 * Start installation of VirtualBox.
 */
bool vboxInstall() {

    return false;
}

/**
 * Tool function to extract the mac address of the VM from the NIC line definition
 */
std::string _vbox_extractMac( std::string nicInfo ) {
    CRASH_REPORT_BEGIN;
    // A nic line is like this:
    // MAC: 08002724ECD0, Attachment: Host-only ...
    size_t iStart = nicInfo.find("MAC: ");
    if (iStart != string::npos ) {
        size_t iEnd = nicInfo.find(",", iStart+5);
        string mac = nicInfo.substr( iStart+5, iEnd-iStart-5 );
        
        // Convert from AABBCCDDEEFF notation to AA:BB:CC:DD:EE:FF
        return mac.substr(0,2) + ":" +
               mac.substr(2,2) + ":" +
               mac.substr(4,2) + ":" +
               mac.substr(6,2) + ":" +
               mac.substr(8,2) + ":" +
               mac.substr(10,2);
               
    } else {
        return "";
    }
    CRASH_REPORT_END;
};

/**
 * Tool function to replace the last part of the given IP
 */
std::string _vbox_changeUpperIP( std::string baseIP, int value ) {
    CRASH_REPORT_BEGIN;
    size_t iDot = baseIP.find_last_of(".");
    if (iDot == string::npos) return "";
    return baseIP.substr(0, iDot) + "." + ntos<int>(value);
    CRASH_REPORT_END;
};