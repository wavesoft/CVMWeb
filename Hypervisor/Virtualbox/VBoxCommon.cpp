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

/**
 * Search Virtualbox on the environment and return an initialized
 * VBoxHypervisor object if it was found.
 */
Hypervisor * vboxDetect() {

	/*
    Hypervisor * hv;
    vector<string> paths;
    string bin, p;
    
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
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p;
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            bin = p + "/VBoxGuestAdditions.iso";
            if (file_exists(bin)) {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->daemonBinPath = "";
            return hv;
        }
        #endif
        #if defined(__APPLE__) && defined(__MACH__)
        bin = p + "/VBoxManage";
        if (file_exists(bin)) {
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p;
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            bin = p + "/VBoxGuestAdditions.iso";
            if (file_exists(bin)) {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->daemonBinPath = "";
            return hv;
        }
        #endif
        #ifdef __linux__
        bin = p + "/bin/VBoxManage";
        if (file_exists(bin)) {
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p + "/bin";
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            
            // (1) Check for additions on XXX/share/virtualbox [/usr, /usr/local]
            bin = p + "/share/virtualbox/VBoxGuestAdditions.iso";
            if (!file_exists(bin)) {

                // (2) Check for additions on XXX/additions [/opt/VirtualBox]
                bin = p + "/additions/VBoxGuestAdditions.iso";
                if (file_exists(bin)) {
                    ((Virtualbox*)hv)->hvGuestAdditions = bin;
                }

            } else {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->daemonBinPath = "";
            return hv;
        }
        #endif
        
    }	
	*/

	return NULL;

}

/**
 * Start installation of VirtualBox.
 */
bool vboxInstall() {

    return false;
}