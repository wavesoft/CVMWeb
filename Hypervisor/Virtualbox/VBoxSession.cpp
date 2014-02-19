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

#include "VBoxSession.h"
#include "VBoxInstance.h"

#include "CVMGlobals.h"

using namespace std;

/////////////////////////////////////
/////////////////////////////////////
////
//// Local tool functions
////
/////////////////////////////////////
/////////////////////////////////////

/**
 *
 * Replace macros
 * 
 * Look for the specified macros in the iString and return a copy
 * where all of them are replaced with the values found in mapData map.
 *
 * This function looks for the following two patterms:
 *
 *  ${name}           : Replace with the value of the given variable name or 
 *                      with an empty string if it's missing.
 *  ${name:default}   : Replace with the variable value or the given default value.
 *
 */
std::string macroReplace( ParameterMapPtr mapData, std::string iString ) {
    CRASH_REPORT_BEGIN;

    // Extract map data to given map
    std::map< std::string, std::string > uData;
    if (mapData) mapData->toMap( &uData );

    // Replace Tokens
    size_t iPos, ePos, lPos = 0, tokStart = 0, tokLen = 0;
    while ( (iPos = iString.find("${", lPos)) != string::npos ) {

        // Find token bounds
//        CVMWA_LOG("Debug", "Found '${' at " << iPos);
        tokStart = iPos;
        iPos += 2;
        ePos = iString.find("}", iPos);
        if (ePos == string::npos) break;
//        CVMWA_LOG("Debug", "Found '}' at " << ePos);
        tokLen = ePos - tokStart;

        // Extract token value
        string token = iString.substr(tokStart+2, tokLen-2);
//        CVMWA_LOG("Debug", "Token is '" << token << "'");
        
        // Extract default
        string vDefault = "";
        iPos = token.find(":");
        if (iPos != string::npos) {
//            CVMWA_LOG("Debug", "Found ':' at " << iPos );
            vDefault = token.substr(iPos+1);
            token = token.substr(0, iPos);
//            CVMWA_LOG("Debug", "Default is '" << vDefault << "', token is '" << token << "'" );
        }

        
        // Look for token value
        string vValue = vDefault;
//        CVMWA_LOG("Debug", "Checking value" );
        if (uData.find(token) != uData.end())
            vValue = uData[token];
        
        // Replace value
//        CVMWA_LOG("Debug", "Value is '" << vValue << "'" );
        iString = iString.substr(0,tokStart) + vValue + iString.substr(tokStart+tokLen+1);
        
        // Move forward
//        CVMWA_LOG("Debug", "String replaced" );
        lPos = tokStart + tokLen;
    }
    
    // Return replaced data
    return iString;
    CRASH_REPORT_END;
};

/**
 * Get last 2kb of the VirtualBox log and try to locate the string that mentions
 * state change.
 */
int getStateFromFile( std::string logPath ) {
    int state = SS_AVAILABLE;

    // Locate Logfile
    string logFile = logPath + "/VBox.log";
    CVMWA_LOG("Debug", "Looking for state change  in " << logFile );
    if (!file_exists(logFile)) return SS_MISSING;

    // Open input stream
    ifstream fIn(logFile.c_str(), ifstream::in);

    // Read as few bytes as possible
    string inBufferLine, stateStr;
    size_t iStart, iEnd, i1, i2, qStart, qEnd;
    char inBuffer[1024];
    fIn.seekg( 2048, fIn.end );
    while (!fIn.eof()) {

        // Read line
        fIn.getline( inBuffer, 1024 );

        // Handle it via higher-level API
        inBufferLine.assign( inBuffer );
        if ((iStart = inBufferLine.find("Changing the VM state from")) != string::npos) {

            // Pick the appropriate ending
            iEnd = inBufferLine.length();
            i1 = inBufferLine.find("\r");
            i2 = inBufferLine.find("\n");
            if (i1 < iEnd) iEnd=i1;
            if (i2 < iEnd) iEnd=i2;

            // Find first quotation
            qStart = inBufferLine.find('\'', iStart);
            if (qStart == string::npos) continue;
            qEnd = inBufferLine.find('\'', qStart+1);
            if (qEnd == string::npos) continue;

            // Find second quotation
            qStart = inBufferLine.find('\'', qEnd+1);
            if (qStart == string::npos) continue;
            qEnd = inBufferLine.find('\'', qStart+1);
            if (qEnd == string::npos) continue;

            // Extract string
            stateStr = inBufferLine.substr( qStart+1, qEnd-qStart-2 );

            // Compare to known state names
            if      (stateStr.compare("RUNNING") == 0) state = SS_RUNNING;
            else if (stateStr.compare("OFF") == 0) state = SS_POWEROFF;
            else if (stateStr.compare("SUSPENDED") == 0) state = SS_PAUSED;

            // If we got 'SAVING' it means the VM was saved
            if      (stateStr.compare("SAVING") == 0) return SS_SAVED;

        }

    }

    // Return the found state
    return state;
}

/**
 * Parse VirtualBox Log file in order to get the launched process PID
 */
int getPIDFromFile( std::string logPath ) {
    int pid = 0;

    // Locate Logfile
    string logFile = logPath + "/VBox.log";
    CVMWA_LOG("Debug", "Looking for PID in " << logFile );
    if (!file_exists(logFile)) return 0;

    // Open input stream
    ifstream fIn(logFile.c_str(), ifstream::in);
    
    // Read as few bytes as possible
    string inBufferLine;
    size_t iStart, iEnd, i1, i2;
    char inBuffer[1024];
    while (!fIn.eof()) {

        // Read line
        fIn.getline( inBuffer, 1024 );

        // Handle it via higher-level API
        inBufferLine.assign( inBuffer );
        if ((iStart = inBufferLine.find("Process ID:")) != string::npos) {

            // Pick the appropriate ending
            iEnd = inBufferLine.length();
            i1 = inBufferLine.find("\r");
            i2 = inBufferLine.find("\n");
            if (i1 < iEnd) iEnd=i1;
            if (i2 < iEnd) iEnd=i2;

            // Extract string
            inBufferLine = inBufferLine.substr( iStart+12, iEnd-iStart );

            // Convert to integer
            pid = ston<int>( inBufferLine );
            break;
        }
    }

    CVMWA_LOG("Debug", "PID extracted from file: " << pid );

    // Close and return PID
    fIn.close();
    return pid;

}

/**
 * Check if the VM logs exist - This means that the VM is still alive
 */
bool vboxLogExists( std::string logPath ) {
    return file_exists( logPath + "/VBox.log" );
}

/////////////////////////////////////
/////////////////////////////////////
////
//// FSM implementation functions 
////
/////////////////////////////////////
/////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialize connection with VirtualBox
 */
void VBoxSession::Initialize() {
    FSMDoing("Initializing session");


    FSMDone("Session initialized");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Load VirtualBox session 
 */
void VBoxSession::UpdateSession() {
    FSMDoing("Loading session information");

    // If VBoxID is missing, directly go to 'destroyed'
    if (!parameters->contains("vboxid")) {
        FSMSkew(3);
        FSMDone("Session has no virtualbox reflection");
        return;
    }

    // Query VM status and fetch local state variable
    map<string, string> info = getMachineInfo();
    int localInitialized = local->getNum<int>("initialized",0);

    // Store machine info
    machine->fromMap( &info, true );

    // If we got an error, the VM is missing
    if (info.find(":ERROR:") != info.end()) {

        if (localInitialized != 0) {

            // If the VM was initialized but now nothing is found,
            // it means that the VM was destroyed externally.
            // Therefore we must flush the local variables
            local->clear();
            
        }

        FSMSkew(3); // Destroyed state
        FSMDone("Virtualbox instance has gone away");

    } else {
        // Route according to state
        if (info.find("State") != info.end()) {

            // Check if we are not initialized
            if (localInitialized == 0) {

                // We are not initialized, so just switch to 'exists' state and
                // let it be re-initialized.
                FSMSkew(8);
                FSMDone("Session is not initialized");

            } else {

                string state = info["State"];
                if (state.find("running") != string::npos) {
                    FSMSkew(7); // Running state
                    FSMDone("Session is running");
                } else if (state.find("paused") != string::npos) {
                    FSMSkew(6); // Paused state
                    FSMDone("Session is paused");
                } else if (state.find("saved") != string::npos) {
                    FSMSkew(5); // Saved state
                    FSMDone("Session is saved");
                } else if (state.find("aborted") != string::npos) {
                    FSMSkew(4); // Aborted is also a 'powered-off' state
                    FSMDone("Session is aborted");
                } else if (state.find("powered off") != string::npos) {
                    FSMSkew(4); // Powered off state
                    FSMDone("Session is powered off");
                } else {
                    // UNKNOWN STATE //
                    CVMWA_LOG("ERROR", "Unknown state");
                }
                
            }

        } else {
            // ERROR //
            CVMWA_LOG("ERROR", "Missing state info");
        }
    }

    FSMDone("Session updated");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Handle errors
 */
void VBoxSession::HandleError() {
    FSMDoing("Handling error");

    FSMDone("Error handled");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Cure errors
 */
void VBoxSession::CureError() {
    FSMDoing("Curing Error");

    FSMDone("Error cured");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Create new VM
 */
void VBoxSession::CreateVM() {
    FSMDoing("Creating Virtual Machine");
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    int ans;

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);

    // Check what kind of VM to create
    string osType = "Linux26";
    if ((flags & HVF_SYSTEM_64BIT) != 0) osType="Linux26_64";

    // Create and register a new VM
    args.str("");
    args << "createvm"
        << " --name \"" << parameters->get("name") << "\""
        << " --ostype " << osType
        << " --register";
    
    // Execute and handle errors
    ans = this->wrapExec(args.str(), &lines, NULL, execConfig);
    if (ans != 0) {
        errorOccured("Unable to create a new virtual machine", HVE_CREATE_ERROR);
        return;
    }
    
    // Parse output
    toks = tokenize( &lines, ':' );
    if (toks.find("UUID") == toks.end()) {
        errorOccured("Unable to detect the VirtualBox ID of the newly allocated VM", HVE_CREATE_ERROR);
        return;
    }

    // Store VBox UUID
    std::string uuid = toks["UUID"];
    parameters->set("vboxid", uuid);


    // Attach an IDE controller
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "IDE"
        << " --add "        << "ide";
    
    ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
    if (ans != 0) {
        // Destroy VM
        destroyVM();
        // Trigger Error
        errorOccured("Unable to attach a new IDE controller", HVE_CREATE_ERROR);
        return;
    }

    // Attach a SATA controller
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "SATA"
        << " --add "        << "sata";
    
    ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
    if (ans != 0) {
        // Destroy VM
        destroyVM();
        // Trigger Error
        errorOccured("Unable to attach a new SATA controller", HVE_CREATE_ERROR);
        return;
    }

    // Attach a floppy controller
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << FLOPPYIO_CONTROLLER
        << " --add "        << "floppy";
    
    ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
    if (ans != 0) {
        // Destroy VM
        destroyVM();
        // Trigger Error
        errorOccured("Unable to attach a new SATA controller", HVE_CREATE_ERROR);
        return;
    }

    // The current (known) VM state is 'created'
    local->set("state", "0");


    FSMDone("Session initialized");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Configure the new VM instace
 */
void VBoxSession::ConfigureVM() {
    FSMDoing("Configuring Virtual Machine");
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string uuid;
    int ans;

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);

    // Find a random free port for VRDE
    int rdpPort = local->getNum<int>("rdpPort", 0);
    if (rdpPort == 0) {
        rdpPort = (rand() % 0xFBFF) + 1024;
        while (isPortOpen( "127.0.0.1", rdpPort ))
            rdpPort = (rand() % 0xFBFF) + 1024;
        local->setNum<int>("rdpPort", rdpPort);
    }

    /* Pick the boot medium depending on the mount type */
    string bootMedium = "dvd";
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) bootMedium = "disk";

    // Modify VM to match our needs
    args.str("");
    args << "modifyvm "
        << parameters->get("vboxid")
        << " --cpus "                   << parameters->get("cpus", "2")
        << " --memory "                 << parameters->get("memory", "1024")
        << " --cpuexecutioncap "        << parameters->get("executionCap", "80")
        << " --vram "                   << "32"
        << " --acpi "                   << "on"
        << " --ioapic "                 << "on"
        << " --vrde "                   << "on"
        << " --vrdeaddress "            << "127.0.0.1"
        << " --vrdeauthtype "           << "null"
        << " --vrdeport "               << rdpPort
        << " --boot1 "                  << bootMedium
        << " --boot2 "                  << "none"
        << " --boot3 "                  << "none" 
        << " --boot4 "                  << "none"
        << " --nic1 "                   << "nat"
        << " --natdnshostresolver1 "    << "on";
    
    // Enable graphical additions if instructed to do so
    if ((flags & HVF_GRAPHICAL) != 0) {
        args << " --draganddrop "       << "hosttoguest"
             << " --clipboard "         << "bidirectional";
    }

    // Setup network
    if ((flags & HVF_DUAL_NIC) != 0) {
        // Create two adapters if DUAL_NIC is specified
        args << " --nic2 "              << "hostonly" << " --hostonlyadapter2 \"" << local->get("hostonlyif") << "\"";
    } else {
        // Otherwise create a NAT rule
        args << " --natpf1 "            << "guestapi,tcp,127.0.0.1," << local->get("apiPort") << ",," << parameters->get("apiPort");
    }

    // Execute and handle errors
    ans = this->wrapExec(args.str(), &lines, NULL, execConfig);
    if (ans != 0) {
        errorOccured("Unable to modify the Virtual Machine", HVE_EXTERNAL_ERROR);
        return;
    }

    // We are initialized
    local->set("initialized","1");

    FSMDone("Virtual Machine configured");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Configure the VM network
 */
void VBoxSession::ConfigNetwork() {

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);
    int ans;

    // Check if we are NATing or if we are using the second NIC 
    if ((flags & HVF_DUAL_NIC) != 0) {

        // =============================================================================== //
        //   NETWORK MODE 1 : Dual Interface                                               //
        // ------------------------------------------------------------------------------- //
        //  In this mode a secondary, host-only adapter will be added to the VM, enabling  //
        //  any kind of traffic to pass through to the VM. The API URL will be built upon  //
        //  this IP address.                                                               //
        // =============================================================================== //

        // Don't touch the host-only interface if we have one already defined
        if (!local->contains("hostonlyif")) {

            // Lookup for the adapter
            string ifHO;
            ans = getHostOnlyAdapter( &ifHO, FSMBegin<FiniteTask>("Configuring VM Network") );
            if (ans != HVE_OK) {
                errorOccured("Unable to pick the appropriate host-only adapter", ans);
                return;
            }

            // Store the host-only adapter name
            local->set("hostonlyif", ifHO);

            // Store the API Port info
            local->set("apiPort", parameters->get("apiPort"));
            local->set("apiHost", "127.0.0.1");

        } else {

            // Just mark the task done
            FSMDone("VM Network configured");

        }

    } else {

        // =============================================================================== //
        //   NETWORK MODE 2 : NAT on the main interface                                    //
        // ------------------------------------------------------------------------------- //
        //  In this mode a NAT port forwarding rule will be added to the first NIC that    //
        //  enables communication only to the specified API port. This is much simpler     //
        //  since the guest IP does not need to be known.                                  //
        // =============================================================================== //

        // Show progress
        FSMDoing("Looking-up for a free API port");

        // Ensure we have a local API Port defined
        int localApiPort = local->getNum<int>("apiPort", 0);
        if (localApiPort == 0) {

            // Find a random free port for API
            localApiPort = (rand() % 0xFBFF) + 1024;
            while (isPortOpen( "127.0.0.1", localApiPort ))
                localApiPort = (rand() % 0xFBFF) + 1024;

            // Store the API Port info
            local->setNum<int>("apiPort", localApiPort);
            local->set("apiHost", "127.0.0.1");

        }

        // Complete FSM
        FSMDone("Network configuration obtained");

    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Download the required media to the application folder
 */
void VBoxSession::DownloadMedia() {
    FiniteTaskPtr pf = FSMBegin<FiniteTask>("Downloading required media");
    if (pf) pf->setMax(1);

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);
    std::string sFilename;
    int ans;

    // ============================================================================= //
    //   MODE 1 : Regular Mode                                                       //
    // ----------------------------------------------------------------------------- //
    //   In this mode the 'cvmVersion' variable contains the URL of a gzipped VMDK   //
    //   image. This image will be downloaded, extracted and placed on cache. Then   //
    //   it will be cloned using copy-on write mode on the user's VM directory.      //
    // ============================================================================= //
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) {

        // Prepare filename and checksum
        string urlFilename = parameters->get("diskURL", "");
        string checksum = parameters->get("diskChecksum", "");

        // If anything of those two is blank, fail
        if (urlFilename.empty() || checksum.empty()) {
            errorOccured("Missing disk and/or checksum parameters", HVE_NOT_VALIDATED);
            return;
        }

        // Check if we are downloading a compressed file
        FiniteTaskPtr pfDownload;
        string urlFilenamePart = getURLFilename(urlFilename);
        if (pf) pfDownload = pf->begin<FiniteTask>("Downloading CernVM ISO");
        if (urlFilenamePart.find(".gz") != std::string::npos) {
            // Download compressed disk
            ans = hypervisor->downloadFileGZ(
                            urlFilename,
                            checksum,
                            &sFilename,
                            pfDownload
                        );
        } else {
            // Download boot disk
            ans = hypervisor->downloadFile(
                            urlFilename,
                            checksum,
                            &sFilename,
                            pfDownload
                        );
        }


        // Validate result
        if (ans != HVE_OK) {
            errorOccured("Unable to download the disk image", ans);
            return;
        }

        // Store boot iso image
        local->set("bootDisk", sFilename);

    }

    // ============================================================================= //
    //   MODE 2 : CernVM-Micro Mode                                                  //
    // ----------------------------------------------------------------------------- //
    //   In this mode a new blank, scratch disk is created. The 'cvmVersion'         //
    //   contains the version of the VM to be downloaded.                            //
    // ============================================================================= //
    else {

        // URL Filename
        string urlFilename = URL_CERNVM_RELEASES "/ucernvm-images." + parameters->get("cernvmVersion", DEFAULT_CERNVM_VERSION)  \
                                + ".cernvm." + parameters->get("cernvmArch", "x86_64") \
                                + "/ucernvm-" + parameters->get("cernvmFlavor", "devel") \
                                + "." + parameters->get("cernvmVersion", DEFAULT_CERNVM_VERSION) \
                                + ".cernvm." + parameters->get("cernvmArch", "x86_64") + ".iso";

        // Download boot disk
        FiniteTaskPtr pfDownload;
        if (pf) pfDownload = pf->begin<FiniteTask>("Downloading CernVM ISO");
        ans = hypervisor->downloadFileURL(
                        urlFilename,
                        urlFilename + ".sha256",
                        &sFilename,
                        pfDownload
                    );

        // Validate result
        if (ans != HVE_OK) {
            errorOccured("Unable to download the CernVM Disk", ans);
            return;
        }

        // Store boot iso image
        local->set("bootISO", sFilename);

    }

    // Complete download task
    if (pf) pf->complete("Required media downloaded");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Configure boot media of the VM
 */
void VBoxSession::ConfigureVMBoot() {
    FSMDoing("Preparing boot medium");
    int ans;

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);

    // ------------------------------------------------
    // MODE 1 : Disk image mode
    // ------------------------------------------------
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) {

        // Get disk path
        string bootDisk = local->get("bootDisk");

        // Mount hdd in boot controller using multi-attach mode
        ans = mountDisk( BOOT_CONTROLLER, BOOT_PORT, BOOT_DEVICE, T_HDD,
                         bootDisk, true );

        // Check result
        if (ans == HVE_ALREADY_EXISTS) {
            FSMDoing("Boot medium already in place");
        } else if (ans == HVE_DELETE_ERROR) {
            errorOccured("Unable to unmount previously mounted boot medium", ans);
            return;
        } else if (ans != HVE_OK) {
            errorOccured("Unable to mount the boot medium", ans);
            return;
        }

    }

    // ------------------------------------------------
    // MODE 2 : Micro-CernVM Mode
    // ------------------------------------------------
    else {

        // Get disk path
        string bootISO = local->get("bootISO");

        // Mount dvddrive in boot controller without multi-attach
        ans = mountDisk( BOOT_CONTROLLER, BOOT_PORT, BOOT_DEVICE, T_DVD,
                         bootISO, false );

        // Check result
        if (ans == HVE_ALREADY_EXISTS) {
            FSMDoing("Boot medium already in place");
        } else if (ans == HVE_DELETE_ERROR) {
            errorOccured("Unable to unmount previously mounted boot medium", ans);
            return;
        } else if (ans != HVE_OK) {
            errorOccured("Unable to mount the boot medium", ans);
            return;
        }

    }

    // ----------------------------------------------
    // Check if we should attach guest additions ISO
    // ----------------------------------------------
    #ifdef GUESTADD_USE
    // Get guest additions ISO file
    string additionsISO = boost::static_pointer_cast<VBoxInstance>(hypervisor)->hvGuestAdditions;
    if ( ((flags & HVF_GUEST_ADDITIONS) != 0) && !additionsISO.empty() ) {
        
        // Mount dvddrive in guest additions controller without multi-attach
        ans = mountDisk( GUESTADD_CONTROLLER, GUESTADD_PORT, GUESTADD_DEVICE, T_DVD,
                         additionsISO, false );

        // Check result
        if (ans == HVE_ALREADY_EXISTS) {
            FSMDoing("Guest additions already in place");
        } else if (ans == HVE_DELETE_ERROR) {
            errorOccured("Unable to unmount previously mounted boot medium", ans);
            return;
        } else if (ans != HVE_OK) {
            errorOccured("Unable to mount the boot medium", ans);
            return;
        }

    }
    #endif

    FSMDone("Boot medium prepared");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release Boot media of the VM
 */
void VBoxSession::ReleaseVMBoot() {
    FSMDoing("Releasing boot medium");

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);

    // Unmount boot disk (don't delete)
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) {
        unmountDisk( BOOT_CONTROLLER, BOOT_PORT, BOOT_DEVICE, T_HDD, false );
    } else {
        unmountDisk( BOOT_CONTROLLER, BOOT_PORT, BOOT_DEVICE, T_DVD, false );
    }

    // If we have guest additions, unmount that ISO too
    #ifdef GUESTADD_USE
    string additionsISO = boost::static_pointer_cast<VBoxInstance>(hypervisor)->hvGuestAdditions;
    if ( ((flags & HVF_GUEST_ADDITIONS) != 0) && !additionsISO.empty() ) {
        unmountDisk( GUESTADD_CONTROLLER, GUESTADD_PORT, GUESTADD_DEVICE, T_DVD, false );
    }
    #endif

    FSMDone("Boot medium released");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allocate a new scratch disk for the VM
 */
void VBoxSession::ConfigureVMScratch() {
    FSMDoing("Preparing scatch storage");
    ostringstream args;
    int ans;

    // Check if we have a scratch disk attached to the machine
    if (!machine->contains(SCRATCH_DSK)) {

        // Create a hard disk for this VM
        string vmDisk = getTmpFile(".vdi", this->getDataFolder());

        // (4) Create disk
        args.str("");
        args << "createhd"
            << " --filename "   << "\"" << vmDisk << "\""
            << " --size "       << parameters->get("disk");

        // Execute and handle errors
        ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
        if (ans != 0) {
            errorOccured("Unable to allocate a scratch disk", HVE_EXTERNAL_ERROR);
            return;
        }

        // Generate a new uuid
        std::string diskGUID = newGUID();

        // Attach disk to the SATA controller
        args.str("");
        args << "storageattach "
            << parameters->get("vboxid")
            << " --storagectl " << SCRATCH_CONTROLLER
            << " --port "       << SCRATCH_PORT
            << " --device "     << SCRATCH_DEVICE
            << " --type "       << "hdd"
            << " --setuuid "    << diskGUID
            << " --medium "     << "\"" << vmDisk << "\"";

        // Execute and handle errors
        ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
        if (ans != 0) {
            errorOccured("Unable to attach the scratch disk", HVE_EXTERNAL_ERROR);
            return;
        }

        // Everything worked as expected.
        // Update disk file path in the scratch disk controller
        machine->set(SCRATCH_DSK, vmDisk + " (UUID: " + diskGUID + ")");

        FSMDone("Scratch storage prepared");
    } else {
        FSMDone("Scratch disk already exists");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release the scratch disk from the VM
 */
void VBoxSession::ReleaseVMScratch() {
    FSMDoing("Releasing scratch storage");

    // Unmount boot disk (and delete)
    unmountDisk( SCRATCH_CONTROLLER, SCRATCH_PORT, SCRATCH_DEVICE, T_HDD, true );

    FSMDone("Scratch storage released");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Check if the mounted VMAPI disk has not changed
 */
void VBoxSession::CheckVMAPI() {
    FSMDoing("Checking VM API medium");

    // Get VMAPI Contents and re-generate the VMAPI Data
    std::string data = getUserData();
    std::string dataRef = local->get("vmapi_contents", "");

    // Check if data are not the same
    if (data.compare(dataRef) != 0) {
        FSMSkew(107);
        FSMDone("VM API medium has changed. Destroying and re-starting the VM");
        return;
    }

    FSMDone("VM API medium does not need to be modified");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Create a VM API disk (ex. floppyIO or OpenNebula ISO)
 */
void VBoxSession::ConfigureVMAPI() {
    FSMDoing("Preparing VM API medium");

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);
    std::string sFilename;
    int ans;

    // ------------------------------------------------
    // MODE 1 : Floppy-IO Contextualization
    // ------------------------------------------------
    if ((flags & HVF_FLOPPY_IO) != 0) {

        // Unmount/remove previous VMAPI floppy
        ans = unmountDisk( FLOPPYIO_CONTROLLER, FLOPPYIO_PORT, FLOPPYIO_DEVICE, T_FLOPPY, true );
        if (ans != HVE_OK) {
            errorOccured("Unable detach previously attached context floppy", HVE_EXTERNAL_ERROR);
            return;
        }

        // Prepare and store the VMAPI data
        std::string data = getUserData();
        local->set("vmapi_contents", data);

        // Create a new floppy disk
        ans = hypervisor->buildFloppyIO( data, &sFilename );
        if (ans != HVE_OK) {
            errorOccured("Unable to create a contextualization floppy disk", HVE_EXTERNAL_ERROR);
            return;
        }

        // Mount the new floppy disk
        ans = mountDisk( FLOPPYIO_CONTROLLER, FLOPPYIO_PORT, FLOPPYIO_DEVICE, T_FLOPPY,
                         sFilename, false );

        // Check result
        if (ans == HVE_ALREADY_EXISTS) {
            FSMDoing("Contextualization floppy already in place");
        } else if (ans == HVE_DELETE_ERROR) {
            errorOccured("Unable to unmount previously mounted contextualization floppy", ans);
            return;
        } else if (ans != HVE_OK) {
            errorOccured("Unable to mount the contextualization floppy", ans);
            return;
        }

    }

    // ------------------------------------------------
    // MODE 2 : ContextISO Contextualization
    // ------------------------------------------------
    else {

        // Unmount/remove previous VMAPI iso
        ans = unmountDisk( CONTEXT_CONTROLLER, CONTEXT_PORT, CONTEXT_DEVICE, T_DVD, true );
        if (ans != HVE_OK) {
            errorOccured("Unable detach previously attached context iso", HVE_EXTERNAL_ERROR);
            return;
        }

        // Prepare and store the VMAPI data
        std::string data = getUserData();
        local->set("vmapi_contents", data);

        // Create a new iso disk
        ans = hypervisor->buildContextISO( data, &sFilename );
        if (ans != HVE_OK) {
            errorOccured("Unable to create a contextualization iso", HVE_EXTERNAL_ERROR);
            return;
        }

        // Mount the new iso disk
        ans = mountDisk( CONTEXT_CONTROLLER, CONTEXT_PORT, CONTEXT_DEVICE, T_DVD,
                         sFilename, false );

        // Check result
        if (ans == HVE_ALREADY_EXISTS) {
            FSMDoing("Contextualization iso already in place");
        } else if (ans == HVE_DELETE_ERROR) {
            errorOccured("Unable to unmount previously mounted contextualization iso", ans);
            return;
        } else if (ans != HVE_OK) {
            errorOccured("Unable to mount the contextualization iso", ans);
            return;
        }

    }

    FSMDone("VM API medium prepared");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release a VM API disk
 */
void VBoxSession::ReleaseVMAPI() {
    FSMDoing("Releasing VM API medium");

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);

    // ------------------------------------------------
    // MODE 1 : Floppy-IO Contextualization
    // ------------------------------------------------
    if ((flags & HVF_FLOPPY_IO) != 0) {
        // Unmount context floppy (and delete)
        unmountDisk( FLOPPYIO_CONTROLLER, FLOPPYIO_PORT, FLOPPYIO_DEVICE, T_FLOPPY, true );
    }

    // ------------------------------------------------
    // MODE 2 : ContextISO Contextualization
    // ------------------------------------------------
    else {
        // Unmount context disk (and delete)
        unmountDisk( CONTEXT_CONTROLLER, CONTEXT_PORT, CONTEXT_DEVICE, T_DVD, true );
    }

    FSMDone("VM API medium released");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Prepare the VM for booting
 */
void VBoxSession::PrepareVMBoot() {
    FSMDoing("Preparing for VM Boot");

    FSMDone("VM Booted");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Destroy the VM instance (remove files and everything)
 */
void VBoxSession::DestroyVM() {
    FSMDoing("Destryoing VM");
    int ans;

    // Destroy VM
    ans = destroyVM();
    if (ans != HVE_OK) {
        errorOccured("Unable to destroy the VM", ans);
        return;
    }

    FSMDone("VM Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Shut down the VM
 */
void VBoxSession::PoweroffVM() {
    FSMDoing("Powering VM off");

    // Poweroff VM
    int ans = controlVM("poweroff");
    if (ans != HVE_OK) {
        errorOccured("Unable to poweroff the VM", ans);
        return;
    }

    FSMDone("VM Powered off");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Discard saved VM state
 */
void VBoxSession::DiscardVMState() {
    FSMDoing("Discarding saved VM state");

    // Discard vm state
    int ans = this->wrapExec("discardstate " + parameters->get("vboxid"), NULL, NULL, execConfig);
    if (ans != 0) {
        errorOccured("Unable to discard the saved VM state", ans);
        return;
    }

    FSMDone("Saved VM state discarted");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Boot the VM
 */
void VBoxSession::StartVM() {
    FSMDoing("Starting VM");

    // Extract flags
    int flags = parameters->getNum<int>("flags", 0);
    int ans;

    // Start VM
    if ((flags & HVF_HEADFUL) != 0) {
        ans = this->wrapExec("startvm " + parameters->get("vboxid") + " --type gui", NULL, NULL, execConfig);
    } else {
        ans = this->wrapExec("startvm " + parameters->get("vboxid") + " --type headless", NULL, NULL, execConfig);
    }

    // Handle errors
    if (ans != 0) {
        errorOccured("Unable to start the VM", ans);
        return;
    }

    // Load machine info
    map<string, string> info = getMachineInfo();
    if (info.find(":ERROR:") == info.end()) {
        machine->fromMap( &info, true );
    }

    // Get PID from the log file
    local->setNum<int>("pid", getPIDFromFile( machine->get("Log folder") ));

    // We are done
    FSMDone("VM Started");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Save the state of the VM
 */
void VBoxSession::SaveVMState() {
    FSMDoing("Saving VM state");

    // Save VM state
    int ans = controlVM("savestate");
    if (ans != HVE_OK) {
        errorOccured("Unable save the VM state", ans);
        return;
    }

    FSMDone("VM State saved");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Put the VM in paused state
 */
void VBoxSession::PauseVM() {
    FSMDoing("Pausing the VM");

    // Pause VM
    int ans = controlVM("pause");
    if (ans != HVE_OK) {
        errorOccured("Unable to pause the VM", ans);
        return;
    }

    FSMDone("VM Paused");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Resume the VM from paused state
 */
void VBoxSession::ResumeVM() {
    FSMDoing("Resuming VM");

    // Resume VM
    int ans = controlVM("resume");
    if (ans != HVE_OK) {
        errorOccured("Unable to resume the VM", ans);
        return;
    }

    FSMDone("VM Resumed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Fatal error sink
 */
void VBoxSession::FatalErrorSink() {
    FSMDoing("Session unable to continue. Cleaning-up");

    // Destroy everything
    destroyVM();

    FSMDone("Session cleaned-up");
}

/////////////////////////////////////
/////////////////////////////////////
////
//// HVSession Implementation
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Initialize a new session
 */
int VBoxSession::open ( ) {
    
    // Start the FSM thread
    FSMThreadStart();

    // Goto SessionUpdate
    FSMGoto(101);

    // We are good
    return HVE_SCHEDULED;
    
}

/**
 * Pause the VM session
 */
int VBoxSession::pause ( ) {

    // Switch to paused state
    FSMGoto(6);

    // Scheduled for execution
    return HVE_SCHEDULED;
}

/**
 * Close the VM session.
 *
 * If the optional parameter 'unmonitored' is specified, the this function
 * shall not wait for the VM to shutdown.
 */
int VBoxSession::close ( bool unmonitored ) {

    // Switch to destroyed state
    FSMGoto(3);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Resume a paused VM
 */
int VBoxSession::resume ( ) {
    
    // Switch to running state
    FSMGoto(7);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Forcefully reboot the VM
 */
int VBoxSession::reset ( ) {
    return HVE_NOT_IMPLEMENTED;    
}

/**
 * Shut down the VM
 */
int VBoxSession::stop ( ) {
    
    // Switch to powerOff state
    FSMGoto(4);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Put the VM to saved state
 */
int VBoxSession::hibernate ( ) {
    
    // Switch to paused state
    FSMGoto(5);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Put the VM to started state
 */
int VBoxSession::start ( std::map<std::string,std::string> *userData ) {

    // Update user data
    this->userData->fromMap( userData, true );

    // Switch to running state
    FSMGoto(7);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Change the execution cap of the VM
 */
int VBoxSession::setExecutionCap ( int cap ) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Set a property to the VM
 */
int VBoxSession::setProperty ( std::string name, std::string key ) {
    properties->set(name, key);
    return HVE_OK;
}

/**
 * Get a property of the VM
 */
std::string VBoxSession::getProperty ( std::string name ) {
    return properties->get(name);
}

/**
 * Build a hostname where the user should connect
 * in order to get the VM's display.
 */
std::string VBoxSession::getRDPAddress ( ) {
    std::ostringstream oss;
    oss << "127.0.0.1:" << local->get("rdpPort");
    return oss.str();
}

/**
 * Return hypervisor-specific extra information
 */
std::string VBoxSession::getExtraInfo ( int extraInfo ) {
    CRASH_REPORT_BEGIN;

    if (extraInfo == EXIF_VIDEO_MODE) {
        CVMWA_LOG("Debug", "Getting video mode")
        map<string, string> info = this->getMachineInfo( 2000 );
        
        for (std::map<string, string>::iterator it=info.begin(); it!=info.end(); ++it) {
            string pname = (*it).first;
            string pvalue = (*it).second;
            CVMWA_LOG("Debug", "getMachineInfo(): '" << pname << "' = '" << pvalue << "'");
        }
        
        if (info.find("Video mode") != info.end())
            return info["Video mode"];
    }

    return "";
    CRASH_REPORT_END;
}

/**
 * Build a hostname where the user should connect
 * in order to interact with the VM instance.
 */
std::string VBoxSession::getAPIHost ( ) {
    return local->get("apiHost");
}

/**
 * Return port number allocated for the API
 */
int VBoxSession::getAPIPort ( ) {
    return local->getNum<int>("apiPort");
}

/**
 * Update the state of the VM, triggering the
 * appropriate state change event callbacks
 */
int VBoxSession::update ( bool waitTillInactive ) {

    // Wait until the FSM is not doing anything
    if (!waitTillInactive) {
        // Exit if the FSM is still active
        if (FSMActive()) 
            return HVE_SCHEDULED;
    }
    FSMWaitInactive();

    // Get current state
    int lastState = local->getNum<int>("state", 0);

    // Re-read the config file from disk
    parameters->sync();

    // Get the new state
    int newState = local->getNum<int>("state", 0);

    // Check for some obvious cases, where the PID has gone
    // away while we expect it to be there - this is much faster
    // than quering VirtualBox.
    if ((newState == SS_PAUSED) || (newState == SS_RUNNING)) {

        // In these cases, the VM is supposed to be running
        if (!isPIDAlive( local->getNum<int>("pid") )) {

            // If the VBox logs are also missing, it means that
            // the VM has gone away
            if (!vboxLogExists( machine->get("Log folder") )) {
                newState = SS_MISSING;

            } else {

                // Try to read the status from the log folder
                int logState = getStateFromFile( machine->get("Log folder") );

                // Check for some valid states from the logfile
                if (logState == SS_POWEROFF) {
                    newState = SS_POWEROFF;
                } else if (logState == SS_SAVED) {
                    newState = SS_SAVED;
                } else {
                    CVMWA_LOG("Error", "Mismatching VM state " << logState << " to " << newState );
                }

            }
        }
    }

    // Handle state switches
    if (newState != lastState) {
        CVMWA_LOG("Debug", "Update state switch from " << lastState << " to " << newState);

        // Handle state switches
        if (newState == SS_MISSING) {
            FSMSkew( 3 ); // Goto 'Destroyed'
        } else if (newState == SS_AVAILABLE) {
            FSMSkew( 8 ); // Goto 'Exists'
        } else if (newState == SS_POWEROFF) {
            FSMSkew( 4 ); // Goto 'Power Off'
        } else if (newState == SS_SAVED) {
            FSMSkew( 5 ); // Goto 'Saved'
        } else if (newState == SS_PAUSED) {
            FSMSkew( 6 ); // Goto 'Paused'
        } else if (newState == SS_RUNNING) {
            FSMSkew( 7 ); // Goto 'Running'
        }

        // The FSM will automatically go to HandleError & CureError if something
        // has gone really wrong.

    }

    // It was OK
    return HVE_OK;
}

/**
 * Abort what we are doing and prepare
 * for reaping.
 */
void VBoxSession::abort ( ) {

    // Stop the FSM thread
    // (This will send an interrupt signal,
    // causing all intermediate code to except)
    FSMThreadStop();

}

/////////////////////////////////////
/////////////////////////////////////
////
//// Event Feedback
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Notification from the VBoxInstance that the session
 * has been forcefully destroyed from an external source.
 */
void VBoxSession::hvNotifyDestroyed () {

    // Stop the FSM thread
    FSMThreadStop();

}

/**
 * Notification from the VBoxInstance that we are going
 * for a forceful shutdown. We should cleanup everything
 * without raising any alert during the handling.
 */
void VBoxSession::hvStop () {

    // Stop the FSM thread
    FSMThreadStop();

}

/////////////////////////////////////
/////////////////////////////////////
////
//// SimpleFSM Overrides
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Notification from the SimpleFSM instance when we enter a state
 */
void VBoxSession::FSMEnteringState( const int state, const bool final ) {

    // On checkpoint states, update the VM state
    // in the local config file.

    if (state == 3) { // Destroyed
        local->setNum<int>( "state", SS_MISSING );
        if (final) this->fire( "stateChanged", ArgumentList( SS_MISSING ) );
    } else if (state == 8) { // Exists
        local->setNum<int>( "state", SS_AVAILABLE );
        if (final) this->fire( "stateChanged", ArgumentList( SS_AVAILABLE ) );
    } else if (state == 4) { // Power off
        local->setNum<int>( "state", SS_POWEROFF );
        if (final) this->fire( "stateChanged", ArgumentList( SS_POWEROFF ) );
    } else if (state == 5) { // Saved state
        local->setNum<int>( "state", SS_SAVED );
        if (final) this->fire( "stateChanged", ArgumentList( SS_SAVED ) );
    } else if (state == 6) { // Paused state
        local->setNum<int>( "state", SS_PAUSED );
        if (final) this->fire( "stateChanged", ArgumentList( SS_PAUSED ) );
    } else if (state == 7) { // Running state
        local->setNum<int>( "state", SS_RUNNING );
        if (final) this->fire( "stateChanged", ArgumentList( SS_RUNNING ) );
    }
}


/////////////////////////////////////
/////////////////////////////////////
////
//// HVSession Tool Functions
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Wrapper to call the appropriate function in the hypervisor and
 * automatically pass the session ID for us.
 */
int VBoxSession::wrapExec ( std::string cmd, std::vector<std::string> * stdoutList, std::string * stderrMsg, const SysExecConfig& config ) {
    CRASH_REPORT_BEGIN;
    return this->hypervisor->exec(cmd, stdoutList, stderrMsg, config );
    CRASH_REPORT_END;
}

/**
 * Destroy and unregister VM
 */
int VBoxSession::destroyVM () {
    CRASH_REPORT_BEGIN;

    // Destroy session
    ostringstream args;
    int ans;

    // Unregister and destroy all VM resources
    args.str("");
    args << "unregistervm"
        << " " << parameters->get("vboxid")
        << " --delete";
    
    // Execute and handle errors
    ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
    if (ans != 0) {
        errorOccured("Unable to destroy the Virtual Machine", HVE_EXTERNAL_ERROR);
        return HVE_EXTERNAL_ERROR;
    }

    // Reset properties
    local->set("initialized","0");
    local->erase("vboxid");

    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Update error info and switch to error state
 */
void VBoxSession::errorOccured ( const std::string & str, int errNo ) {
    CRASH_REPORT_BEGIN;

    // Update error info
    errorCode = errNo;
    errorMessage = str;

    // Notify progress failure on the FSM progress
    FSMFail( str, errNo );

    // Check the timestamp of the last time we had an error
    unsigned long currTime = getMillis();
    if ((currTime - errorTimestamp) < SESSION_HEAL_THRESSHOLD ) {
        errorCount += 1;
        if (errorCount > SESSION_HEAL_TRIES) {
            CVMWA_LOG("Error", "Too many errors. Won't try to heal them again");
            FSMGoto(112);
        } else {
            // Skew through the error state, while trying to head
            // towards the previously defined state.
            FSMSkew( 2 );
        }
    } else {
        errorCount = 1;

        // Skew through the error state, while trying to head
        // towards the previously defined state.
        FSMSkew( 2 );
    }

    // Update last error timestamp
    errorTimestamp = currTime;
    CRASH_REPORT_END;
}

/**
 *  Compile the user data and return it's string representation
 */
std::string VBoxSession::getUserData ( ) {
    CRASH_REPORT_BEGIN;
    std::string patchedUserData = parameters->get("userData", "");

    // Update local userData
    if ( !patchedUserData.empty() ) {
        patchedUserData = macroReplace( userData, patchedUserData );
    }

    // Return user data
    return patchedUserData;
    CRASH_REPORT_END;
}

/**
 * Unmount a medium from the VirtulaBox Instance
 */
int VBoxSession::unmountDisk ( const std::string & controller, 
                               const std::string & port, 
                               const std::string & device, 
                               const VBoxDiskType & dtype, 
                               const bool deleteFile ) {
    CRASH_REPORT_BEGIN;
    ostringstream args;
    string kk, kv;
    int ans;

    // Calculate the name of the disk slot
    std::string DISK_SLOT = controller + " (" + port + ", " + device + ")";

    // String-ify the disk type
    std::string type;
    if (dtype == T_HDD) type="disk";
    else if (dtype == T_DVD) type="dvd";
    else if (dtype == T_FLOPPY) type="floppy";

    // Unmount disk only if it's already mounted
    if (machine->contains( DISK_SLOT, true )) {

        // Otherwise unmount the existing disk
        args.str("");
        args << "storageattach "
            << parameters->get("vboxid")
            << " --storagectl " << controller
            << " --port "       << port
            << " --device "     << device
            << " --medium "     << "none";

        // Execute and handle errors
        ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
        if (ans != HVE_OK) return ans;

        // If we are also asked to erase the file, do it now
        if (deleteFile) {

            // Split on '('
            // (Line contents is something like "IDE (1, 0): image.vmdk (UUID: ...)")
            getKV( machine->get(DISK_SLOT), &kk, &kv, '(', 0 );
            kk = kk.substr(0, kk.length()-1);

            // Close and unregister medium
            args.str("");
            args << "closemedium " << type << " "
                << "\"" << kk << "\" --delete";

            // Execute and handle errors
            ans = this->wrapExec(args.str(), NULL, NULL, execConfig);
            if (ans != HVE_OK) return ans;

            // Remove file
            ::remove( kk.c_str() );

        }

        // Remove file from the mounted devices list
        machine->erase( DISK_SLOT );

    }

    // Already done
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * (Re-)Mount a disk on the specified controller
 * This function automatically unmounts a previously attached disk if the filenames
 * do not match.
 */
int VBoxSession::mountDisk ( const std::string & controller, 
                             const std::string & port, 
                             const std::string & device, 
                             const VBoxDiskType & dtype,
                             const std::string & diskFile, 
                             bool multiAttach ) {
    CRASH_REPORT_BEGIN;

    vector<string> lines;
    ostringstream args;
    string kk, kv;
    int ans;

    // Switch multiAttach to false if we are not using 'hdd' type
    if (multiAttach && (dtype != T_HDD)) {
        multiAttach = false;
    }

    // String-ify the disk type
    std::string type;
    if (dtype == T_HDD) type="hdd";
    else if (dtype == T_DVD) type="dvddrive";
    else if (dtype == T_FLOPPY) type="floppy";

    // Calculate the name of the disk slot
    std::string DISK_SLOT = controller + " (" + port + ", " + device + ")";

    // (A) Unmount previously mounted disk if it's not what we want
    if (machine->contains( DISK_SLOT )) {
        
        // Split on '('
        // (Line contents is something like "IDE (1, 0): image.vmdk (UUID: ...)")
        getKV( machine->get(DISK_SLOT), &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        if (kk.compare( diskFile ) == 0) {

            // If the file is the one we want, we are done            
            return HVE_ALREADY_EXISTS;

        } else {

            // Otherwise unmount the existing disk
            ans = unmountDisk( controller, port, device, dtype, false );
            if (ans != HVE_OK) return HVE_DELETE_ERROR;

        }

    }

    // Prepare two locations where we can find the disk: By filename and by UUID.
    // That's because before some version VirtualBox we need the disk UUID, while for others we need the full path
    string masterDiskPath = "\"" + diskFile + "\"";
    string masterDiskUUID = "";
    
    // If we are doing multi-attach, try to use UUID-based mounting
    if (multiAttach) {
        // Get a list of the disks in order to properly compute multi-attach 
        vector< map< string, string > > disks = boost::static_pointer_cast<VBoxInstance>(hypervisor)->getDiskList();
        for (vector< map<string, string> >::iterator i = disks.begin(); i != disks.end(); i++) {
            map<string, string> disk = *i;
            // Look of the master disk of what we are using
            if ( (disk.find("Type") != disk.end()) && (disk.find("Parent UUID") != disk.end()) && (disk.find("Location") != disk.end()) && (disk.find("UUID") != disk.end()) ) {
                // Check if all the component maches
                if ( (disk["Type"].compare("multiattach") == 0) && (disk["Parent UUID"].compare("base") == 0) && samePath(disk["Location"],diskFile) ) {
                    // Use the master UUID instead of the filename
                    CVMWA_LOG("Info", "Found master with UUID " << disk["UUID"]);
                    masterDiskUUID = "{" + disk["UUID"] + "}";
                    break;
                }
            }
        }
    }

    // Generate a new uuid
    std::string diskGUID = newGUID();

    // (B.1) Try to attach disk to the SATA controller using full path
    args.str("");
    args << "storageattach "
        << parameters->get("vboxid")
        << " --storagectl " << controller
        << " --port "       << port
        << " --device "     << device
        << " --type "       << type
        << " --setuuid "    << diskGUID
        << " --medium "     <<  masterDiskPath;

    // Append multiattach flag if we are instructed to do so
    if (multiAttach)
        args << " --mtype " << "multiattach";

    // Execute
    ans = this->wrapExec(args.str(), &lines, NULL, execConfig);

    // If we are using multi-attach, try to mount by UUID if mounting
    // by filename has failed
    if (multiAttach && !masterDiskUUID.empty()) {

        // If it was OK, just return
        if (ans == 0) {
            // Update mounted medium info
            machine->set( DISK_SLOT, diskFile + " (UUID: " + diskGUID + ")" );
            return HVE_OK;
        }
        
        // (B.2) Try to attach disk to the SATA controller using UUID (For older VirtualBox versions)
        args.str("");
        args << "storageattach "
            << parameters->get("vboxid")
            << " --storagectl " << controller
            << " --port "       << port
            << " --device "     << device
            << " --type "       << type
            << " --mtype "      << "multiattach"
            << " --setuuid "    << diskGUID
            << " --medium "     <<  masterDiskUUID;

        // Execute
        ans = this->wrapExec(args.str(), &lines, NULL, execConfig);

    }

    // Update mounted medium info if it was OK
    if (ans == HVE_OK) {
        machine->set( DISK_SLOT, diskFile + " (UUID: " + diskGUID + ")" );
    }

    // Retun last execution result
    return ans;

    CRASH_REPORT_END;
}


/**
 * Return the folder where we can store the VM disks.
 */
std::string VBoxSession::getDataFolder ( ) {
    CRASH_REPORT_BEGIN;

    // If we already have a path, return it
    if (!this->dataPath.empty())
        return this->dataPath;

    // Find configuration folder
    if (machine->contains("Config file")) {
        string settingsFolder = machine->get("Config file");

        // Strip quotation marks
        if ((settingsFolder[0] == '"') || (settingsFolder[0] == '\''))
            settingsFolder = settingsFolder.substr( 1, settingsFolder.length() - 2);

        // Strip the settings file (leave path) and store it on dataPath
        this->dataPath = stripComponent( settingsFolder );
    }

    // Return folder
    return this->dataPath;

    CRASH_REPORT_END;
}

/**
 * Return or create a new host-only adapter.
 */
int VBoxSession::getHostOnlyAdapter ( std::string * adapterName, const FiniteTaskPtr & fp ) {
    CRASH_REPORT_BEGIN;

    vector<string> lines;
    vector< map<string, string> > ifs;
    vector< map<string, string> > dhcps;
    string ifName = "", vboxName, ipServer, ipMin, ipMax;
    
    // Progress update
    if (fp) fp->setMax(4);

    /////////////////////////////
    // [1] Check for interfaces
    /////////////////////////////

    // Check if we already have host-only interfaces
    if (fp) fp->doing("Enumerating host-only adapters");
    int ans = this->wrapExec("list hostonlyifs", &lines, NULL, execConfig);
    if (ans != 0) {
        if (fp) fp->fail("Unable to enumerate the host-only adapters", HVE_QUERY_ERROR);
        return HVE_QUERY_ERROR;
    }
    if (fp) fp->done("Got adapter list");
    
    // Check if there is really nothing
    if (lines.size() == 0) {

        // Create adapter
        if (fp) fp->doing("Creating missing host-only adapter");
        ans = this->wrapExec("hostonlyif create", NULL, NULL, execConfig);
        if (ans != 0) {
            if (fp) fp->fail("Unable to create a host-only adapter", HVE_CREATE_ERROR);
            return HVE_CREATE_ERROR;
        }
    
        // Repeat check
        if (fp) fp->doing("Validating created host-only adapter");
        ans = this->wrapExec("list hostonlyifs", &lines, NULL, execConfig);
        if (ans != 0) {
            if (fp) fp->fail("Unable to enumerate the host-only adapters", HVE_QUERY_ERROR);
            return HVE_QUERY_ERROR;
        }
        
        // Still couldn't pick anything? Error!
        if (lines.size() == 0) {
            if (fp) fp->fail("Unable to verify the creation of the host-only adapter", HVE_NOT_VALIDATED);
            return HVE_NOT_VALIDATED;
        }

        if (fp) fp->done("Adapter created");
    } else {
        if (fp) fp->done("Adapter exists");

    }

    // Fetch the interface in existance
    ifs = tokenizeList( &lines, ':' );
    
    /////////////////////////////
    // [2] Check for DHCP
    /////////////////////////////

    // Dump the DHCP server states
    if (fp) fp->doing("Checking for DHCP server in the interface");
    ans = this->wrapExec("list dhcpservers", &lines, NULL, execConfig);
    if (ans != 0) {
        if (fp) fp->fail("Unable to enumerate the host-only adapters", HVE_QUERY_ERROR);
        return HVE_QUERY_ERROR;
    }

    // Parse DHCP server info
    dhcps = tokenizeList( &lines, ':' );
    
    // Initialize DHCP lookup variables
    bool    foundDHCPServer = false;
    string  foundIface      = "",
            foundBaseIP     = "",
            foundVBoxName   = "",
            foundMask       = "";

    // Process interfaces
    for (vector< map<string, string> >::iterator i = ifs.begin(); i != ifs.end(); i++) {
        map<string, string> iface = *i;

        CVMWA_LOG("log", "Checking interface");
        mapDump(iface);

        // Ensure proper environment
        if (iface.find("Name") == iface.end()) continue;
        if (iface.find("VBoxNetworkName") == iface.end()) continue;
        if (iface.find("IPAddress") == iface.end()) continue;
        if (iface.find("NetworkMask") == iface.end()) continue;
        
        // Fetch interface info
        ifName = iface["Name"];
        vboxName = iface["VBoxNetworkName"];
        
        // Check if we have DHCP enabled on this interface
        bool hasDHCP = false;
        for (vector< map<string, string> >::iterator i = dhcps.begin(); i != dhcps.end(); i++) {
            map<string, string> dhcp = *i;
            if (dhcp.find("NetworkName") == dhcp.end()) continue;
            if (dhcp.find("Enabled") == dhcp.end()) continue;

            CVMWA_LOG("log", "Checking dhcp");
            mapDump(dhcp);
            
            // The network has a DHCP server, check if it's running
            if (vboxName.compare(dhcp["NetworkName"]) == 0) {
                if (dhcp["Enabled"].compare("Yes") == 0) {
                    hasDHCP = true;
                    break;
                    
                } else {
                    
                    // Make sure the server has a valid IP address
                    bool updateIPInfo = false;
                    if (dhcp["IP"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["lowerIPAddress"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["upperIPAddress"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["NetworkMask"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (updateIPInfo) {
                        
                        // Prepare IP addresses
                        ipServer = _vbox_changeUpperIP( iface["IPAddress"], 100 );
                        ipMin = _vbox_changeUpperIP( iface["IPAddress"], 101 );
                        ipMax = _vbox_changeUpperIP( iface["IPAddress"], 254 );
                    
                        // Modify server
                        ans = this->wrapExec(
                            "dhcpserver modify --ifname \"" + ifName + "\"" +
                            " --ip " + ipServer +
                            " --netmask " + iface["NetworkMask"] +
                            " --lowerip " + ipMin +
                            " --upperip " + ipMax
                             , NULL, NULL, execConfig);
                        if (ans != 0) continue;
                    
                    }
                    
                    // Check if we can enable the server
                    ans = this->wrapExec("dhcpserver modify --ifname \"" + ifName + "\" --enable", NULL, NULL, execConfig);
                    if (ans == 0) {
                        hasDHCP = true;
                        break;
                    }
                    
                }
            }
        }
        
        // Keep the information of the first interface found
        if (foundIface.empty()) {
            foundIface = ifName;
            foundVBoxName = vboxName;
            foundBaseIP = iface["IPAddress"];
            foundMask = iface["NetworkMask"];
        }
        
        // If we found DHCP we are done
        if (hasDHCP) {
            foundDHCPServer = true;
            break;
        }
        
    }

    // Information obtained
    if (fp) fp->done("DHCP information recovered");

    
    // If there was no DHCP server, create one
    if (!foundDHCPServer) {
        if (fp) fp->doing("Adding a DHCP Server");
        
        // Prepare IP addresses
        ipServer = _vbox_changeUpperIP( foundBaseIP, 100 );
        ipMin = _vbox_changeUpperIP( foundBaseIP, 101 );
        ipMax = _vbox_changeUpperIP( foundBaseIP, 254 );
        
        // Add and start server
        ans = this->wrapExec(
            "dhcpserver add --ifname \"" + foundIface + "\"" +
            " --ip " + ipServer +
            " --netmask " + foundMask +
            " --lowerip " + ipMin +
            " --upperip " + ipMax +
            " --enable"
             , NULL, NULL, execConfig);

        if (ans != 0) {
            if (fp) fp->fail("Unable to add a DHCP server on the interface", HVE_CREATE_ERROR);
            return HVE_CREATE_ERROR;
        }
                
    } else {
        if (fp) fp->done("DHCP Server is running");
    }
    
    // Got my interface
    if (fp) fp->complete("Interface found");
    *adapterName = foundIface;
    return HVE_OK;

    CRASH_REPORT_END;
}

/**
 * Return the properties of the VM.
 */
std::map<std::string, std::string> VBoxSession::getMachineInfo ( int retries, int timeout ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;

    // Local SysExecConfig
    SysExecConfig config(execConfig);
    config.retries = retries;
    config.timeout = timeout;
    
    /* Perform property update */
    int ans = this->wrapExec("showvminfo "+this->parameters->get("vboxid"), &lines, NULL, config);
    if (ans != 0) {
        dat[":ERROR:"] = ntos<int>( ans );
        return dat;
    }
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
    CRASH_REPORT_END;
}

/**
 * Launch the VM
 */
int VBoxSession::startVM ( ) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Send control commands to the VM.
 */
int VBoxSession::controlVM ( std::string how, int timeout ) {
    CRASH_REPORT_BEGIN;

    // Local SysExecConfig
    SysExecConfig config(execConfig);
    config.timeout = timeout;

    int ans = this->wrapExec("controlvm " + parameters->get("vboxid") + " " + how, NULL, NULL, config);
    if (ans != 0) return HVE_CONTROL_ERROR;
    return 0;
    CRASH_REPORT_END;
}
