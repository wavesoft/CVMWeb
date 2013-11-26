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

#include "CVMGlobals.h"
#include "global/config.h"

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
int vboxInstall( const DownloadProviderPtr & downloadProvider, const UserInteractionPtr & ui, const FiniteTaskPtr & pf, int retries ) {
    CRASH_REPORT_BEGIN;
    const int maxSteps = 200;
    HVInstancePtr hv;
    int res;
    string tmpHypervisorInstall;
    string checksum;

    // Initialize progress feedback
    if (pf) {
        pf->setMax(5);
    }

    ////////////////////////////////////
    // Contact the information point
    ////////////////////////////////////
    string requestBuf;

    // Download trials
    for (int tries=0; tries<retries; tries++) {
        CVMWA_LOG( "Info", "Fetching data" );

        // Send progress feedback
        if (pf) pf->doing("Downloading hypervisor configuration");

        // Try to download the configuration URL
        res = downloadProvider->downloadText( URL_HYPERVISOR_CONFIG FBSTRING_PLUGIN_VERSION, &requestBuf );
        if ( res != HVE_OK ) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );

                // Send progress feedback
                if (pf) pf->doing("Re-downloading hypervisor configuration");

                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Too many retries while downloading hypervisor configuration");

            return res;

        } else {

            // Send progress feedback
            if (pf) pf->done("Downloaded hypervisor configuration");

            /* Reached this point, we are good to continue */
            break;            
        }

    }
    
    ////////////////////////////////////
    // Extract information
    ////////////////////////////////////

    // Prepare variables
    vector<string> lines;
    splitLines( requestBuf, &lines );
    map<string, string> data = tokenize( &lines, '=' );
    
    // Pick the URLs to download from
    #ifdef _WIN32
    const string kDownloadUrl = "win32";
    const string kChecksum = "win32-sha256";
    const string kInstallerName = "win32-installer";
    const string kFileExt = ".exe";
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    const string kDownloadUrl = "osx";
    const string kChecksum = "osx-sha256";
    const string kInstallerName = "osx-installer";
    const string kFileExt = ".dmg";
    #endif
    #ifdef __linux__
    
    // Do some more in-depth analysis of the linux platform
    LINUX_INFO linuxInfo;
    getLinuxInfo( &linuxInfo );

    // Detect
    #if defined(__LP64__) || defined(_LP64)
    string kDownloadUrl = "linux64-" + linuxInfo.osDistID;
    #else
    string kDownloadUrl = "linux32-" + linuxInfo.osDistID;
    #endif
    
    // Calculate keys for other installers
    string kChecksum = kDownloadUrl + "-sha256";
    string kInstallerName = kDownloadUrl + "-installer";
    
    CVMWA_LOG( "Info", "Download URL key = '" << kDownloadUrl << "'"  );
    CVMWA_LOG( "Info", "Checksum key = '" << kChecksum << "'"  );
    CVMWA_LOG( "Info", "Installer key = '" << kInstallerName << "'"  );
    
    #endif
    
    ////////////////////////////////////
    // Verify information
    ////////////////////////////////////

    // Verify that the keys we are looking for exist
    if (data.find( kDownloadUrl ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No download URL data found" );

        // Send progress fedback
        if (pf) pf->fail("No hypervisor download URL data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kChecksum ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No checksum data found" );

        // Send progress fedback
        if (pf) pf->fail("No setup checksum data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kInstallerName ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No installer program data found" );

        // Send progress fedback
        if (pf) pf->fail("No installer program data found");

        return HVE_EXTERNAL_ERROR;
    }
    
    
    #ifdef __linux__
    // Pick an extension and installation type based on the installer= parameter
    int installerType = PMAN_NONE;
    string kFileExt = ".run";
    if (data[kInstallerName].compare("dpkg") == 0) {
        installerType = PMAN_DPKG; /* Using debian installer */
        kFileExt = ".deb";
    } else if (data[kInstallerName].compare("yum") == 0) {
        installerType = PMAN_YUM; /* Using 'yum localinstall <package> -y' */
        kFileExt = ".rpm";
    }
    #endif

    ////////////////////////////////////
    // Download hypervisor installer
    ////////////////////////////////////

    // Prepare feedback pointers
    VariableTaskPtr downloadPf;
    if (pf) {
        downloadPf = pf->begin<VariableTask>("Downloading hypervisor installer");
    }

    // Download trials loop
    for (int tries=0; tries<retries; tries++) {

        // Download installer
        tmpHypervisorInstall = getTmpFile( kFileExt );
        CVMWA_LOG( "Info", "Downloading " << data[kDownloadUrl] << " to " << tmpHypervisorInstall  );
        res = downloadProvider->downloadFile( data[kDownloadUrl], tmpHypervisorInstall, downloadPf );
        CVMWA_LOG( "Info", "    : Got " << res  );
        if ( res != HVE_OK ) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Unable to download hypervisor installer");
            return res;
        }
        
        // Validate checksum
        if (pf) pf->doing("Validating download");
        sha256_file( tmpHypervisorInstall, &checksum );

        CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data[kChecksum]  );
        if (checksum.compare( data[kChecksum] ) != 0) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Celeanup
            ::remove( tmpHypervisorInstall.c_str() );

            // Send progress fedback
            if (pf) pf->fail("Unable to validate hypervisor installer");
            return HVE_NOT_VALIDATED;
        }

        // Send progress feedback
        if (pf) pf->done("Hypervisor installer downloaded");

        // ( Reached this point, we are good to continue )
        break;

    }
    
    ////////////////////////////////////
    // OS-Dependant installation process
    ////////////////////////////////////
    
    // Prepare feedback pointers
    FiniteTaskPtr installerPf;
    if (pf) {
        installerPf = pf->begin<FiniteTask>("Installing hypervisor");
    }

    // Start installer with retries
    string errorMsg;
    for (int tries=0; tries<retries; tries++) {
        #if defined(__APPLE__) && defined(__MACH__)
            if (installerPf) installerPf->setMax(4, false);

            CVMWA_LOG( "Info", "Attaching" << tmpHypervisorInstall );
            if (installerPf) installerPf->doing("Mouting hypervisor DMG disk");
            res = sysExec("/usr/bin/hdiutil", "attach " + tmpHypervisorInstall, &lines, &errorMsg);
            if (res != 0) {
                if (tries<retries) {
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    if (installerPf) installerPf->doing("Retrying installation");
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to use hdiutil to mount DMG");

                return HVE_EXTERNAL_ERROR;
            }
            if (installerPf) installerPf->done("Mounted DMG disk");

            string infoLine = lines.back();
            string dskDev, dskVolume, extra;
            getKV( infoLine, &dskDev, &extra, ' ', 0);
            getKV( extra, &extra, &dskVolume, ' ', dskDev.size()+1);
            CVMWA_LOG( "Info", "Got disk '" << dskDev << "', volume: '" << dskVolume  );
    
            if (installerPf) installerPf->doing("Starting installer");
            CVMWA_LOG( "Info", "Installing using " << dskVolume << "/" << data[kInstallerName]  );
            res = sysExec("/usr/bin/open", "-W " + dskVolume + "/" + data[kInstallerName], NULL, &errorMsg);
            if (res != 0) {

                CVMWA_LOG( "Info", "Detaching" );
                if (installerPf) installerPf->doing("Unmounting DMG");
                res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg);
                if (tries<retries) {
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    if (installerPf) installerPf->doing("Restarting installer");
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }
            if (installerPf) installerPf->done("Installed hypervisor");

            CVMWA_LOG( "Info", "Detaching" );
            if (installerPf) installerPf->doing("Cleaning-up");
            res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg);
            if (installerPf) {
                installerPf->done("Cleaning-up completed");
                installerPf->complete("Installed hypervisor");
            }

        #elif defined(_WIN32)
            if (installerPf) installerPf->setMax(2, false);

            // Start installer
            if (installerPf) installerPf->doing("Starting installer");
            CVMWA_LOG( "Info", "Starting installer" );

            // CreateProcess does not work because we need elevated permissions,
            // use the classic ShellExecute to run the installer...
            SHELLEXECUTEINFOA shExecInfo = {0};
            shExecInfo.cbSize = sizeof( SHELLEXECUTEINFO );
            shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExecInfo.hwnd = NULL;
            shExecInfo.lpVerb = NULL;
            shExecInfo.lpFile = (LPCSTR)tmpHypervisorInstall.c_str();
            shExecInfo.lpParameters = (LPCSTR)"";
            shExecInfo.lpDirectory = NULL;
            shExecInfo.nShow = SW_SHOWNORMAL;
            shExecInfo.hInstApp = NULL;

            // Validate handle
            if ( !ShellExecuteExA( &shExecInfo ) ) {
                cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }

            // Validate hProcess
            if (shExecInfo.hProcess == 0) {
                cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }

            // Wait for termination
            WaitForSingleObject( shExecInfo.hProcess, INFINITE );
            if (installerPf) installerPf->done("Installer completed");

            // Complete
            if (installerPf) installerPf->complete("Installed hypervisor");

        #elif defined(__linux__)
            if (installerPf) installerPf->setMax(5, false);

            // Check if our environment has what the installer needs
            if (installerPf) installerPf->doing("Probing environment");
            if ((installerType != PMAN_NONE) && (installerType != linuxInfo.osPackageManager )) {
                cout << "ERROR: OS does not have the required package manager (type=" << installerType << ")" << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Re-probing environment");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to probe the environment");

                return HVE_NOT_FOUND;
            }
            if (installerPf) installerPf->done("Probed environment");

            // (1) If we have xdg-open, use it to prompt the user using the system's default GUI
            // ----------------------------------------------------------------------------------
            if (linuxInfo.hasXDGOpen) {

                if (installerPf) installerPf->doing("Starting hypervisor installer");
                string cmdline = "/usr/bin/xdg-open \"" + tmpHypervisorInstall + "\"";
                res = system( cmdline.c_str() );
                if (res < 0) {
                    cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to start the hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Installer started");
            
                // At some point the process that xdg-open launches is
                // going to open the file in order to read it's contnets. 
                // Wait for 10 sec for it to happen
                if (installerPf) installerPf->doing("Waiting for the installation to begin");
                if (!waitFileOpen( tmpHypervisorInstall, true, 60000 )) { // 1 min until it's captured
                    cout << "ERROR: Could not wait for file handler capture: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Waiting again for the installation to begin");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to check the status of the installation");
                    
                    return HVE_STILL_WORKING;
                }
                if (installerPf) installerPf->done("Installation started");

                // Wait for it to be released
                if (installerPf) installerPf->doing("Waiting for the installation to complete");
                if (!waitFileOpen( tmpHypervisorInstall, false, 900000 )) { // 15 mins until it's released
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Waiting again for the installation to complete");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // We can't remove the file, it's in use :(
                    CVMWA_LOG("Error", "ERROR: Could not wait for file handler release: " << res );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to check the status of the installation");

                    return HVE_STILL_WORKING;
                }

                // Done
                if (installerPf) installerPf->done("Installation completed");
        
                // Complete
                if (installerPf) installerPf->complete("Installed hypervisor");

            // (2) If we have GKSudo, do directly dpkg/yum install
            // ------------------------------------------------------
            } else if (linuxInfo.hasGKSudo) {
                string cmdline = "/bin/sh '" + tmpHypervisorInstall + "'";
                if ( installerType == PMAN_YUM ) {
                    cmdline = "/usr/bin/yum localinstall '" + tmpHypervisorInstall + "' -y";
                } else if ( installerType == PMAN_DPKG ) {
                    cmdline = "/usr/bin/dpkg -i '" + tmpHypervisorInstall + "'";
                }

                // Use GKSudo to invoke the cmdline
                if (installerPf) installerPf->doing("Starting installer");
                cmdline = "/usr/bin/gksudo \"" + cmdline + "\"";
                res = system( cmdline.c_str() );
                if (res < 0) {
                    cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to start the hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Installer completed");

                // Complete
                if (installerPf) installerPf->complete("Installed hypervisor");
        
            /* (3) Otherwise create a bash script and prompt the user */
            } else {
            
                /* TODO: I can't do much here :( */
                return HVE_NOT_IMPLEMENTED;
            
            }
        
        #endif
    
        // Give 5 seconds as a cool-down delay
        sleepMs(5000);

        // Check if it was successful
        hv = detectHypervisor();
        if (!hv) {
            CVMWA_LOG( "Info", "ERROR: Could not install hypervisor!" );
            if (tries<retries) {
                if (installerPf) installerPf->restart("Re-trying hypervisor installation");
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Hypervisor installation seems not feasible");

            return HVE_NOT_VALIDATED;
        } else {

            /* Everything worked like a charm */
            ::remove( tmpHypervisorInstall.c_str() );
            break;

        }
        
    }

    /**
     * If we are installing VirtualBox, make sure the VBOX Extension pack are installed
     */
     /*
    if ((hv != NULL) && (hv->type == HV_VIRTUALBOX)) {
        if ( !((Virtualbox*)hv)->hasExtPack() ) {

            // Install extension pack, and fail on error
            if ( (res = ((Virtualbox*)hv)->installExtPack( versionID, downloadProvider, cbProgress, 110, maxSteps, maxSteps )) != HVE_OK ) {
                freeHypervisor(hv);
                return res;
            };

        }
    }
    */

    // Completed
    if (pf) pf->complete("Hypervisor installed successfully");
    return HVE_OK;
    
    CRASH_REPORT_END;
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