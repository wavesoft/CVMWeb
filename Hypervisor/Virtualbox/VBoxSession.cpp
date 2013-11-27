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

/**
 * Initialize connection with VirtualBox
 */
void VBoxSession::Initialize() {
    FSMDoing("Initializing session");

    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    FSMDone("Session initialized");
}

/**
 * Load VirtualBox session 
 */
void VBoxSession::UpdateSession() {
    FSMDoing("Loading session information");

    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    FSMSkew(3);

    FSMDone("Session updated");
}

/**
 * Handle errors
 */
void VBoxSession::HandleError() {
    FSMDoing("Handling error");

    FSMDone("Error handled");
}

/**
 * Cure errors
 */
void VBoxSession::CureError() {
    FSMDoing("Curing Error");

    FSMDone("Error cured");
}

/**
 * Create new VM
 */
void VBoxSession::CreateVM() {
    FSMDoing("Creating Virtual Machine");
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    FSMDone("Session initialized");
}

/**
 * Configure the new VM instace
 */
void VBoxSession::ConfigureVM() {
    FSMDoing("Configuring Virtual Machine");
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    FSMDone("Virtual Machine configured");
}

/**
 * Download the required media to the application folder
 */
void VBoxSession::DownloadMedia() {
    FSMDoing("Downloading required media");
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

    FSMDone("Media downloaded");
}

/**
 * Configure boot media of the VM
 */
void VBoxSession::ConfigureVMBoot() {
    FSMDoing("Preparing boot medium");
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));

    FSMDone("Boot medium prepared");
}

/**
 * Release Boot media of the VM
 */
void VBoxSession::ReleaseVMBoot() {
    FSMDoing("Releasing boot medium");
    cout << "- ReleaseVMBoot" << endl;

    FSMDone("Boot medium released");
}

/**
 * Allocate a new scratch disk for the VM
 */
void VBoxSession::ConfigureVMScratch() {
    FSMDoing("Preparing scatch storage");
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    FSMDone("Scratch storage prepared");
}

/**
 * Release the scratch disk from the VM
 */
void VBoxSession::ReleaseVMScratch() {
    FSMDoing("Releasing scratch storage");

    FSMDone("Scratch storage released");
}

/**
 * Create a VM API disk (ex. floppyIO or OpenNebula ISO)
 */
void VBoxSession::ConfigureVMAPI() {
    FSMDoing("Preparing VM API medium");

    FSMDone("VM API medium prepared");
}

/**
 * Release a VM API disk
 */
void VBoxSession::ReleaseVMAPI() {
    FSMDoing("Releasing VM API medium");

    FSMDone("VM API medium released");
}

/**
 * Prepare the VM for booting
 */
void VBoxSession::PrepareVMBoot() {
    FSMDoing("Preparing for VM Boot");

    FSMDone("VM Booted");
}

/**
 * Destroy the VM instance (remove files and everything)
 */
void VBoxSession::DestroyVM() {
    FSMDoing("Destryoing VM");

    FSMDone("VM Destroyed");
}

/**
 * Shut down the VM
 */
void VBoxSession::PoweroffVM() {
    FSMDoing("Powering VM off");

    FSMDone("VM Powered off");
}

/**
 * Discard saved VM state
 */
void VBoxSession::DiscardVMState() {
    FSMDoing("Discarding saved VM state");

    FSMDone("Saved VM state discarted");
}

/**
 * Boot the VM
 */
void VBoxSession::StartVM() {
    FSMDoing("Starting VM");

    FSMDone("VM Started");
}

/**
 * Save the state of the VM
 */
void VBoxSession::SaveVMState() {
    FSMDoing("Saving VM state");

    FSMDone("VM State saved");
}

/**
 * Put the VM in paused state
 */
void VBoxSession::PauseVM() {
    FSMDoing("Pausing the VM");

    FSMDone("VM Paused");
}

/**
 * Resume the VM from paused state
 */
void VBoxSession::ResumeVM() {
    FSMDoing("Resuming VM");

    FSMDone("VM Resumed");
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
std::string VBoxSession::getProperty ( std::string name, bool forceUpdate ) {
    return "";
}

/**
 * Build a hostname where the user should connect
 * in order to get the VM's display.
 */
std::string VBoxSession::getRDPHost ( ) {
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
 * Simmilar to the update() function, but we are
 * just interested about the state of the VM.
 *
 * TODO: Wha??
 */
int VBoxSession::updateFast ( ) {
    return HVE_NOT_IMPLEMENTED;
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
    return HVE_NOT_IMPLEMENTED;
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
std::string VBoxSession::getHostOnlyAdapter ( ) {
    return "";
}

/**
 * Return the properties of the VM.
 */
std::map<std::string, std::string> VBoxSession::getMachineInfo ( int timeout ) {
    std::map<std::string, std::string> info;

    return info;
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
