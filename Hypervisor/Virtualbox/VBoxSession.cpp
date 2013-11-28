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
 * where all of them are replaced with the values found in uData map.
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

    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    FSMDone("Session initialized");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Load VirtualBox session 
 */
void VBoxSession::UpdateSession() {
    FSMDoing("Loading session information");

    // Get VM status
    map<string, string> info = getMachineInfo();

    // If we got an error, the VM is missing
    if (info.find(":ERROR:") != info.end()) {
        FSMSkew(3); // Destroyed state
    } else {
        // Route according to state
        if (info.find("State") != info.end()) {
            string state = info["State"];
            if (state.find("running") != string::npos) {
                FSMSkew(7); // Running state
            } else if (state.find("paused") != string::npos) {
                FSMSkew(6); // Paused state
            } else if (state.find("saved") != string::npos) {
                FSMSkew(5); // Saved state
            } else if (state.find("aborted") != string::npos) {
                FSMSkew(4); // Aborted is also a 'powered-off' state
            } else if (state.find("powered off") != string::npos) {
                FSMSkew(4); // Powered off state
            } else {
                // UNKNOWN STATE //
                CVMWA_LOG("ERROR", "Unknown state");
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
    string uuid;
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
    ans = this->wrapExec(args.str(), &lines);
    if (ans != 0) {
        errorOccured("Unable to create a new virtual machine", HVE_EXTERNAL_ERROR);
        return;
    }
    
    // Parse output
    toks = tokenize( &lines, ':' );
    if (toks.find("UUID") == toks.end()) {
        errorOccured("Unable to detect the VirtualBox ID of the newly allocated VM", HVE_EXTERNAL_ERROR);
        return;
    }
    uuid = toks["UUID"];


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
    int rdpPort = (rand() % 0xFBFF) + 1024;
    while (isPortOpen( "127.0.0.1", rdpPort ))
        rdpPort = (rand() % 0xFBFF) + 1024;
    parameters->setNum<int>("rdpPort", rdpPort);

    /* Pick the boot medium depending on the mount type */
    string bootMedium = "dvd";
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) bootMedium = "disk";

    // Modify VM to match our needs
    args.str("");
    args << "modifyvm "
        << uuid
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
        args << " --nic2 "              << "hostonly" << " --hostonlyadapter2 \"" << parameters->get("hostonlyif") << "\"";
    } else {
        // Otherwise create a NAT rule
        args << " --natpf1 "            << "guestapi,tcp,127.0.0.1," << local->get("apiPort") << ",," << parameters->get("apiPort");
    }


    // Execute and handle errors
    ans = this->wrapExec(args.str(), &lines);
    if (ans != 0) {
        errorOccured("Unable to modify the Virtual Machine", HVE_EXTERNAL_ERROR);
        return;
    }

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
        if (!parameters->contains("hostonlyif")) {

            // Lookup for the adapter
            string ifHO;
            ans = getHostOnlyAdapter( &ifHO, FSMBegin<FiniteTask>("Configuring VM Network") );
            if (ans != HVE_OK) {
                errorOccured("Unable to pick the appropriate host-only adapter", ans);
                return;
            }

            // Store the host-only adapter name
            parameters->set("hostonlyif", ifHO);

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

            // Store the NAT info
            local->setNum<int>("apiPort", localApiPort);

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

        // Download boot disk
        ans = hypervisor->downloadFile(
                        urlFilename,
                        checksum,
                        &sFilename,
                        pf->begin<FiniteTask>("Downloading disk image")
                    );

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
        ans = hypervisor->downloadFileURL(
                        urlFilename,
                        urlFilename + ".sha256",
                        &sFilename,
                        pf->begin<FiniteTask>("Downloading CernVM ISO")
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
    
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));

    FSMDone("Boot medium prepared");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release Boot media of the VM
 */
void VBoxSession::ReleaseVMBoot() {
    FSMDoing("Releasing boot medium");
    cout << "- ReleaseVMBoot" << endl;

    FSMDone("Boot medium released");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allocate a new scratch disk for the VM
 */
void VBoxSession::ConfigureVMScratch() {
    FSMDoing("Preparing scatch storage");
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    FSMDone("Scratch storage prepared");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release the scratch disk from the VM
 */
void VBoxSession::ReleaseVMScratch() {
    FSMDoing("Releasing scratch storage");

    FSMDone("Scratch storage released");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Create a VM API disk (ex. floppyIO or OpenNebula ISO)
 */
void VBoxSession::ConfigureVMAPI() {
    FSMDoing("Preparing VM API medium");

    FSMDone("VM API medium prepared");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Release a VM API disk
 */
void VBoxSession::ReleaseVMAPI() {
    FSMDoing("Releasing VM API medium");

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

    FSMDone("VM Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Shut down the VM
 */
void VBoxSession::PoweroffVM() {
    FSMDoing("Powering VM off");

    FSMDone("VM Powered off");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Discard saved VM state
 */
void VBoxSession::DiscardVMState() {
    FSMDoing("Discarding saved VM state");

    FSMDone("Saved VM state discarted");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Boot the VM
 */
void VBoxSession::StartVM() {
    FSMDoing("Starting VM");

    FSMDone("VM Started");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Save the state of the VM
 */
void VBoxSession::SaveVMState() {
    FSMDoing("Saving VM state");

    FSMDone("VM State saved");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Put the VM in paused state
 */
void VBoxSession::PauseVM() {
    FSMDoing("Pausing the VM");

    FSMDone("VM Paused");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Resume the VM from paused state
 */
void VBoxSession::ResumeVM() {
    FSMDoing("Resuming VM");

    FSMDone("VM Resumed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Fatal error sink
 */
void VBoxSession::FatalErrorSink() {
    FSMDoing("Session unable to continue. Cleaning-up");

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

    // Goto Initialize
    FSMGoto(100);

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
    *(this->userData->parameters.get()) = *userData;

    // Switch to running state
    FSMGoto(7);

    // Scheduled for execution
    return HVE_SCHEDULED;

}

/**
 * Change the execution cap of the VM
 */
int VBoxSession::setExecutionCap ( int cap) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Set a property to the VM
 */
int VBoxSession::setProperty ( std::string name, std::string key ) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Get a property of the VM
 */
std::string VBoxSession::getProperty ( std::string name ) {
    return "";
}

/**
 * Build a hostname where the user should connect
 * in order to get the VM's display.
 */
std::string VBoxSession::getRDPAddress ( ) {
    return "";
}

/**
 * Build a hostname where the user should connect
 * in order to interact with the VM instance.
 */
std::string VBoxSession::getAPIHost ( ) {
    return "";
}

/**
 * Return hypervisor-specific extra information
 */
std::string VBoxSession::getExtraInfo ( int extraInfo ) {
    return "";
}

/**
 * Return port number allocated for the API
 *
 * TODO: Wha??
 */
int VBoxSession::getAPIPort ( ) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Update the state of the VM, triggering the
 * appropriate state change event callbacks
 */
int VBoxSession::update ( ) {
    return HVE_NOT_IMPLEMENTED;
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
//// HVSession Tool Functions
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Wrapper to call the appropriate function in the hypervisor and
 * automatically pass the session ID for us.
 */
int VBoxSession::wrapExec ( std::string cmd, std::vector<std::string> * stdoutList, std::string * stderrMsg, int retries, int timeout ) {
    return this->hypervisor->exec(cmd, stdoutList, stderrMsg, retries, timeout );
}

/**
 * Update error info and switch to error state
 */
void VBoxSession::errorOccured ( const std::string & str, int errNo ) {

    // Update error info
    errorCode = errNo;
    errorMessage = str;

    // Notify progress failure on the FSM progress
    FSMFail( str, errNo );

    // Skew through the error state, while trying to head
    // towards the previously defined state.
    FSMSkew( 2 );

    // Check the timestamp of the last time we had an error
    unsigned long currTime = getMillis();
    if ((errorTimestamp - currTime) < SESSION_HEAL_THRESSHOLD ) {
        errorCount += 1;
        if (errorCount > SESSION_HEAL_TRIES) {
            CVMWA_LOG("Error", "Too many errors. Won't try to heal them again");
            FSMGoto(112);
        }
    } else {
        errorCount = 1;
    }

    // Update last error timestamp
    errorTimestamp = currTime;

}

/**
 *  Compile the user data and return it's string representation
 */
std::string VBoxSession::getUserData ( ) {
    std::string patchedUserData = parameters->get("userData", "");

    // Update local userData
    if ( !patchedUserData.empty() ) {
        patchedUserData = macroReplace( userData, patchedUserData );
    }

    // Return user data
    return patchedUserData;

}

/**
 * Return the UUID of a VM with the specified name and flags. If no
 * such VM exists, allocate a slot for a new one and return that ID.
 */
int VBoxSession::getMachineUUID ( std::string mname, std::string * ans_uuid,  int flags ) {
    return HVE_NOT_IMPLEMENTED;
}

/**
 * Return the folder where we can store the VM disks.
 */
std::string VBoxSession::getDataFolder ( ) {
    return "";
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
    int ans = this->wrapExec("list hostonlyifs", &lines);
    if (ans != 0) {
        if (fp) fp->fail("Unable to enumerate the host-only adapters", HVE_QUERY_ERROR);
        return HVE_QUERY_ERROR;
    }
    if (fp) fp->done("Got adapter list");
    
    // Check if there is really nothing
    if (lines.size() == 0) {

        // Create adapter
        if (fp) fp->doing("Creating missing host-only adapter");
        ans = this->wrapExec("hostonlyif create", NULL, NULL, 2);
        if (ans != 0) {
            if (fp) fp->fail("Unable to create a host-only adapter", HVE_CREATE_ERROR);
            return HVE_CREATE_ERROR;
        }
    
        // Repeat check
        if (fp) fp->doing("Validating created host-only adapter");
        ans = this->wrapExec("list hostonlyifs", &lines, NULL, 2);
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
    ans = this->wrapExec("list dhcpservers", &lines);
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
                             , NULL, NULL, 2);
                        if (ans != 0) continue;
                    
                    }
                    
                    // Check if we can enable the server
                    ans = this->wrapExec("dhcpserver modify --ifname \"" + ifName + "\" --enable", NULL);
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
             , NULL);

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
std::map<std::string, std::string> VBoxSession::getMachineInfo ( int timeout ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->wrapExec("showvminfo "+this->parameters->get("vboxid"), &lines, NULL, 4, timeout);
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
    return HVE_NOT_IMPLEMENTED;
}
