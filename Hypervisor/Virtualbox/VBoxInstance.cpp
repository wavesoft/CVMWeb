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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include "CVMGlobals.h"
#include "global/config.h"

#include "VBoxInstance.h"
#include "Common/Hypervisor.h"
#include "Common/Utilities.h"

using namespace std;

/** =========================================== **\
            Virtualbox Implementation
\** =========================================== **/

/** 
 * Return virtual machine information
 */
map<string, string> VBoxInstance::getMachineInfo( std::string uuid, int timeout ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;
    string err;
    
    // Local exec config
    SysExecConfig config(execConfig);
    config.timeout = timeout;

    // Perform property update
    int ans;
    NAMED_MUTEX_LOCK( uuid );
    ans = this->exec("showvminfo "+uuid, &lines, &err, config );
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) {
        dat[":ERROR:"] = ntos<int>( ans );
        return dat;
    }
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
    CRASH_REPORT_END;
};

/**
 * Return all the properties of the guest
 */
map<string, string> VBoxInstance::getAllProperties( string uuid ) {
    CRASH_REPORT_BEGIN;
    map<string, string> ans;
    vector<string> lines;
    string errOut;

    // Get guest properties
    NAMED_MUTEX_LOCK( uuid );
    if (this->exec( "guestproperty enumerate "+uuid, &lines, &errOut, execConfig ) == 0) {
        for (vector<string>::iterator it = lines.begin(); it < lines.end(); ++it) {
            string line = *it;

            /* Find the anchor locations */
            size_t kBegin = line.find("Name: ");
            if (kBegin == string::npos) continue;
            size_t kEnd = line.find(", value:");
            if (kEnd == string::npos) continue;
            size_t vEnd = line.find(", timestamp:");
            if (vEnd == string::npos) continue;

            /* Get key */
            kBegin += 6;
            string vKey = line.substr( kBegin, kEnd - kBegin );

            /* Get value */
            size_t vBegin = kEnd + 9;
            string vValue = line.substr( vBegin, vEnd - vBegin );

            /* Store values */
            ans[vKey] = vValue;

        }
    }
    NAMED_MUTEX_UNLOCK;

    return ans;
    CRASH_REPORT_END;
}

/**
 * Load sessions if they are not yet loaded
 */
bool VBoxInstance::waitTillReady( const FiniteTaskPtr & pf, const UserInteractionPtr & ui ) {
    CRASH_REPORT_BEGIN;
    
    // Update progress
    if (pf) pf->setMax(2);
    
    // Session loading takes time, so instead of blocking the plugin
    // at creation time, use this mechanism to delay-load it when first accessed.
    if (!this->sessionLoaded) {

        // Create a progress feedback for the session loading
        FiniteTaskPtr pfLoading;
        if (pf) pfLoading = pf->begin<FiniteTask>("Loading sessions");

        // Load sessions
        this->loadSessions( pfLoading );
        this->sessionLoaded = true;

    } else {
        if (pf) pf->done("Sessions are loaded");
    }
    
    // By the way, check if we have the extension pack installed
    if (!this->hasExtPack()) {

        // Create a progress feedback instance for the installer
        FiniteTaskPtr pfInstall;
        if (pf) pfInstall = pf->begin<FiniteTask>("Installing extension pack");

        // Extension pack is released under PUEL license
        // require the user to confirm before continuing
        if (ui) {
            if (ui->confirmLicense("VirtualBox Personal Use and Evaluation License (PUEL)", VBOX_PUEL_LICENSE) != UI_OK) {
                // (User did not click OK)

                // Send error
                if (pf) pf->fail("User denied Oracle PUEL license");

                // Abort
                return false;
            }
        }

        // Start extension pack installation
        this->installExtPack(
                this->downloadProvider,
                pfInstall
            );

    } else {
        if (pf) pf->done("Extension pack is installed");
    }

    if (pf) pf->complete("Hypervisor is ready");

    /**
     * All's good!
     */
    return true;
    CRASH_REPORT_END;
}

/**
 * Return a property from the VirtualBox guest
 */
std::string VBoxInstance::getProperty( std::string uuid, std::string name ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    string value;
    string err;
    
    /* Invoke property query */
    int ans;
    NAMED_MUTEX_LOCK( uuid );
    ans = this->exec("guestproperty get "+uuid+" \""+name+"\"", &lines, &err, execConfig);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return "";
    if (lines.empty()) return "";
    
    /* Process response */
    value = lines[0];
    if (value.substr(0,6).compare("Value:") == 0) {
        return value.substr(7);
    } else {
        return "";
    }
    
    CRASH_REPORT_END;
}

/**
 * Return Virtualbox sessions instead of classic
 */
HVSessionPtr VBoxInstance::allocateSession() {
    CRASH_REPORT_BEGIN;
    
    // Allocate a new GUID for this session
    std::string guid = newGUID();

    // Fetch a config object
    LocalConfigPtr cfg = LocalConfig::forRuntime( "vbsess-" + guid );
    cfg->set("uuid", guid);

    // Return new session instance
    VBoxSessionPtr session = boost::make_shared< VBoxSession >( cfg, this->shared_from_this() );
    
    // Store on session registry and return session object
    this->sessions[ guid ] = session;
    return static_cast<HVSessionPtr>(session);

    CRASH_REPORT_END;
}

/**
 * Load capabilities
 */
int VBoxInstance::getCapabilities ( HVINFO_CAPS * caps ) {
    CRASH_REPORT_BEGIN;
    map<string, string> data;
    vector<string> lines, parts;
    string err;
    int v;
    
    // List the CPUID information
    int ans;
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list hostcpuids", &lines, &err, execConfig);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return HVE_QUERY_ERROR;
    if (lines.empty()) return HVE_EXTERNAL_ERROR;
    
    // Process lines
    for (vector<string>::iterator i = lines.begin(); i != lines.end(); i++) {
        string line = *i;
        if (trimSplit( &line, &parts, " \t", " \t") == 0) continue;
        if (parts[0].compare("00000000") == 0) { // Leaf 0 -> Vendor
            v = hex_ston<int>( parts[2] ); // EBX
            caps->cpu.vendor[0] = v & 0xFF;
            caps->cpu.vendor[1] = (v & 0xFF00) >> 8;
            caps->cpu.vendor[2] = (v & 0xFF0000) >> 16;
            caps->cpu.vendor[3] = (v & 0xFF000000) >> 24;
            v = hex_ston<int>( parts[4] ); // EDX
            caps->cpu.vendor[4] = v & 0xFF;
            caps->cpu.vendor[5] = (v & 0xFF00) >> 8;
            caps->cpu.vendor[6] = (v & 0xFF0000) >> 16;
            caps->cpu.vendor[7] = (v & 0xFF000000) >> 24;
            v = hex_ston<int>( parts[3] ); // ECX
            caps->cpu.vendor[8] = v & 0xFF;
            caps->cpu.vendor[9] = (v & 0xFF00) >> 8;
            caps->cpu.vendor[10] = (v & 0xFF0000) >> 16;
            caps->cpu.vendor[11] = (v & 0xFF000000) >> 24;
            caps->cpu.vendor[12] = '\0';
            
        } else if (parts[0].compare("00000001") == 0) { // Leaf 1 -> Features
            caps->cpu.featuresA = hex_ston<int>( parts[3] ); // ECX
            caps->cpu.featuresB = hex_ston<int>( parts[4] ); // EDX
            v = hex_ston<int>( parts[1] ); // EAX
            caps->cpu.stepping = v & 0xF;
            caps->cpu.model = (v & 0xF0) >> 4;
            caps->cpu.family = (v & 0xF00) >> 8;
            caps->cpu.type = (v & 0x3000) >> 12;
            caps->cpu.exmodel = (v & 0xF0000) >> 16;
            caps->cpu.exfamily = (v & 0xFF00000) >> 20;
            
        } else if (parts[0].compare("80000001") == 0) { // Leaf 80000001 -> Extended features
            caps->cpu.featuresC = hex_ston<int>( parts[3] ); // ECX
            caps->cpu.featuresD = hex_ston<int>( parts[4] ); // EDX
            
        }
    }
    
    // Update flags
    caps->cpu.hasVM = false; // Needs MSR to detect
    caps->cpu.hasVT = 
        ( (caps->cpu.featuresA & 0x20) != 0 ) || // Intel 'vmx'
        ( (caps->cpu.featuresC & 0x2)  != 0 );   // AMD 'svm'
    caps->cpu.has64bit =
        ( (caps->cpu.featuresC & 0x20000000) != 0 ); // Long mode 'lm'
        
    // List the system properties
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list systemproperties", &lines, &err, execConfig);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return HVE_QUERY_ERROR;
    if (lines.empty()) return HVE_EXTERNAL_ERROR;

    // Default limits
    caps->max.cpus = 1;
    caps->max.memory = 1024;
    caps->max.disk = 2048;
    
    // Tokenize into the data map
    parseLines( &lines, &data, ":", " \t", 0, 1 );
    if (data.find("Maximum guest RAM size") != data.end()) 
        caps->max.memory = ston<int>(data["Maximum guest RAM size"]);
    if (data.find("Virtual disk limit (info)") != data.end()) 
        caps->max.disk = ston<long>(data["Virtual disk limit (info)"]) / 1024;
    if (data.find("Maximum guest CPU count") != data.end()) 
        caps->max.cpus = ston<int>(data["Maximum guest CPU count"]);
    
    // Ok!
    return HVE_OK;
    CRASH_REPORT_END;
};

/**
 * Get a list of mediums managed by VirtualBox
 */
std::vector< std::map< std::string, std::string > > VBoxInstance::getDiskList() {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    std::vector< std::map< std::string, std::string > > resMap;
    string err;

    // List the running VMs in the system
    int ans;
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list hdds", &lines, &err, execConfig);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return resMap;
    if (lines.empty()) return resMap;

    // Tokenize lists
    resMap = tokenizeList( &lines, ':' );
    return resMap;
    CRASH_REPORT_END;
}

/**
 * Parse VirtualBox Log file in order to get the launched process PID
 */
int __getPIDFromFile( std::string logPath ) {
    int pid = 0;

    /* Locate Logfile */
    string logFile = logPath + "/VBox.log";
    CVMWA_LOG("Debug", "Looking for PID in " << logFile );
    if (!file_exists(logFile)) return 0;

    /* Open input stream */
    ifstream fIn(logFile.c_str(), ifstream::in);
    
    /* Read as few bytes as possible */
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

/*
int VBoxInstance::updateSession( HVSession * session, bool fast ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    string err;
    bool prevEditable;

    // Don't update session if we are in middle of something
    if ((session->state == STATE_STARTING) || (session->state == STATE_OPPENING) || ((VBoxSession*)session)->updateLock)
        return HVE_INVALID_STATE;
    
    // Get session's uuid
    string uuid = session->uuid;
    if (uuid.empty()) return HVE_USAGE_ERROR;
    
    // Collect details
    map<string, string> info = this->getMachineInfo( uuid, 2000 );
    if (info.empty()) 
        return HVE_NOT_FOUND;
    if (info.find(":ERROR:") != info.end())
        return HVE_IO_ERROR;
    
    // Reset flags
    prevEditable = session->editable;
    session->flags = 0;
    session->editable = true;
    
    // Check state
    session->state = STATE_OPEN;
    if (info.find("State") != info.end()) {
        string state = info["State"];
        if (state.find("running") != string::npos) {
            session->state = STATE_STARTED;
        } else if (state.find("paused") != string::npos) {
            session->state = STATE_PAUSED;
            session->editable = false;
        } else if (state.find("saved") != string::npos) {
            session->state = STATE_OPEN;
            session->editable = false;
        } else if (state.find("aborted") != string::npos) {
            session->state = STATE_OPEN;
        }
    }
    
    // If session switched to editable state, commit pending property changes
    if (session->editable && !prevEditable && !((VBoxSession*)session)->unsyncedProperties.empty()) {
        for (std::map<string, string>::iterator it=((VBoxSession*)session)->unsyncedProperties.begin(); 
                it!=((VBoxSession*)session)->unsyncedProperties.end(); ++it) {
            string name = (*it).first;
            string value = (*it).second;
            
            // Session is now editable
            session->setProperty(name, value);
        }
    }
    
    // Reset property maps
    ((VBoxSession*)session)->unsyncedProperties.clear();
    ((VBoxSession*)session)->properties.clear();
    
    // Get CPU
    if (info.find("Number of CPUs") != info.end()) {
        session->cpus = ston<int>( info["Number of CPUs"] );
    }
    
    // Check flags
    if (info.find("Guest OS") != info.end()) {
        string guestOS = info["Guest OS"];
        if (guestOS.find("64 bit") != string::npos) {
            session->flags |= HVF_SYSTEM_64BIT;
        }
    }

    // Find configuration folder
    if (info.find("Guest OS") != info.end()) {
        string settingsFolder = info["Settings file"];

        // Skip empty files
        if (!settingsFolder.empty()) {

            // Strip quotation marks
            if ((settingsFolder[0] == '"') || (settingsFolder[0] == '\''))
                settingsFolder = settingsFolder.substr( 1, settingsFolder.length() - 2);

            // Strip the settings file (leave path) and store it on dataPath
            ((VBoxSession*)session)->dataPath = stripComponent( settingsFolder );

        } else {

            // No data path found? Use system's temp directory
            ((VBoxSession*)session)->dataPath = getTmpDir();

        }

    }

    // Parse RDP info
    if (info.find("VRDE") != info.end()) {
        string rdpInfo = info["VRDE"];

        // Example line: 'enabled (Address 127.0.0.1, Ports 39211, MultiConn: off, ReuseSingleConn: off, Authentication type: null)'
        if (rdpInfo.find("enabled") != string::npos) {
            size_t pStart = rdpInfo.find("Ports ");
            if (pStart == string::npos) {
                ((VBoxSession *)session)->rdpPort = 0;
                CVMWA_LOG("Debug", "VRDE 'Ports' anchor not found");

            } else {
                pStart += 6;
                size_t pEnd = rdpInfo.find(",", pStart);
                string rdpPort = rdpInfo.substr( pStart, pEnd-pStart );

                // Apply the rdp port
                ((VBoxSession *)session)->rdpPort = ston<int>(rdpPort);
                CVMWA_LOG("Debug", "VRDE Port is " << rdpPort);
            }
        } else {
            ((VBoxSession *)session)->rdpPort = 0;
            CVMWA_LOG("Debug", "VRDE not enabled");
        }
    } else {
        CVMWA_LOG("Debug", "VRDE config not found");
    }
    
    // Parse memory
    if (info.find("Memory size") != info.end()) {
        session->cpus = ston<int>( info["Number of CPUs"] );

        // Parse memory
        string mem = info["Memory size"];
        mem = mem.substr(0, mem.length()-2);
        session->memory = ston<int>(mem);

    }
    
    // Parse CernVM Version from the ISO
    session->version = DEFAULT_CERNVM_VERSION;
    if (info.find( BOOT_DSK ) != info.end()) {

        // Get the filename of the iso
        getKV( info[ BOOT_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        // Extract CernVM Version from file
        session->version = this->cernVMVersion( kk );
        if (session->version.empty()) { 
            // (If we could not find a version number it's a disk-deployment)
            session->version = getFilename( kk );
            session->flags |= HVF_DEPLOYMENT_HDD;
        }
    }
    
    // Check if there are guest additions mounted and update flags
    if (info.find( GUESTADD_DSK ) != info.end()) {
        
        // Get the filename of the iso
        getKV( info[ GUESTADD_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        // Check if we have the guest additions disk
        if ( kk.compare(this->hvGuestAdditions) == 0 ) 
            session->flags |= HVF_GUEST_ADDITIONS;
        
    }
    
    // Check if we have floppy adapter. If we do, it means we are using floppyIO -> updateFlags
    if (info.find( FLOPPYIO_ENUM_NAME ) != info.end()) {
        session->flags |= HVF_FLOPPY_IO;
    }
    
    // Parse disk size
    session->disk = 1024;
    if (info.find( SCRATCH_DSK ) != info.end()) {

        // Get the filename of the iso
        getKV( info[ SCRATCH_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        // Collect disk info
        int ans;
        NAMED_MUTEX_LOCK(kk);
        ans = this->exec("showhdinfo \""+kk+"\"", &lines, &err, 2, 2000);
        NAMED_MUTEX_UNLOCK;
        if (ans == 0) {
        
            // Tokenize data
            diskinfo = tokenize( &lines, ':' );
            if (diskinfo.find("Logical size") != diskinfo.end()) {
                kk = diskinfo["Logical size"];
                kk = kk.substr(0, kk.length()-7); // Strip " MBytes"
                session->disk = ston<int>(kk);
            }
            
        }
    }
    
    // Parse execution cap
    session->executionCap = 100;
    if (info.find( "CPU exec cap" ) != info.end()) {
        
        // Get execution cap
        kk = info["CPU exec cap"];
        kk = kk.substr(0, kk.length()-1); // Strip %
        
        // Convert to number
        session->executionCap = ston<int>( kk );
        
    }
    
    // If we want to be fast, skip time-consuming operations
    if (!fast) {

        // Parse all properties concurrently
        map<string, string> allProps = this->getAllProperties( uuid );

        if (allProps.find("/CVMWeb/daemon/controlled") == allProps.end()) {
            session->daemonControlled = false;
        } else {
            session->daemonControlled = (allProps["/CVMWeb/daemon/controlled"].compare("1") == 0);
        }

        if (allProps.find("/CVMWeb/daemon/cap/min") == allProps.end()) {
            session->daemonMinCap = 0;
        } else {
            session->daemonMinCap = ston<int>(allProps["/CVMWeb/daemon/cap/min"]);
        }

        if (allProps.find("/CVMWeb/daemon/cap/max") == allProps.end()) {
            session->daemonMaxCap = 0;
        } else {
            session->daemonMaxCap = ston<int>(allProps["/CVMWeb/daemon/cap/max"]);
        }

        if (allProps.find("/CVMWeb/daemon/flags") == allProps.end()) {
            session->daemonFlags = 0;
        } else {
            session->daemonFlags = ston<int>(allProps["/CVMWeb/daemon/flags"]);
        }

        if (allProps.find("/CVMWeb/userData") == allProps.end()) {
            //session->userData = "";
        } else {
            //session->userData = base64_decode(allProps["/CVMWeb/userData"]);
        }

        if (allProps.find("/CVMWeb/localApiPort") == allProps.end()) {
            ((VBoxSession *)session)->localApiPort = 0;
        } else {
            ((VBoxSession *)session)->localApiPort = ston<int>(allProps["/CVMWeb/localApiPort"]);
        }
        CVMWA_LOG("Debug", "LocalAPI Port = " << ((VBoxSession *)session)->localApiPort);

        // Get hypervisor pid from file
        if (info.find("Log folder") != info.end())
            session->pid = __getPIDFromFile( info["Log folder"] );
        
        // Store allProps to properties
        ((VBoxSession*)session)->properties = allProps;

    }

    // Updated successfuly
    return HVE_OK;
    CRASH_REPORT_END;
}
*/

/**
 * Return a VirtualBox Session based on the VirtualBox UUID specified
 */
HVSessionPtr VBoxInstance::sessionByVBID ( const std::string& virtualBoxGUID ) {
    CRASH_REPORT_BEGIN;

    // Look for a session with the given GUID
    for (std::map< std::string,HVSessionPtr >::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSessionPtr sess = (*i).second;
        if (sess->parameters->get("vboxid", "").compare( virtualBoxGUID ) == 0 ) {
            return sess;
        }
    }

    // Return an unitialized HVSessionPtr if nothing is found
    return HVSessionPtr();
    CRASH_REPORT_END;
}

HVSessionPtr VBoxInstance::sessionOpen ( const ParameterMapPtr& parameters, const FiniteTaskPtr & pf ) {

    // Call parent function to open session
    HVSessionPtr  sess = HVInstance::sessionOpen(parameters,pf);
    VBoxSessionPtr vbs = boost::static_pointer_cast<VBoxSession>( sess );

    // Set progress feedack object
    vbs->FSMUseProgress( pf, "Updating VM information" );

    // Open session
    vbs->open();

    // Return instance
    return vbs;

}


/**
 * Load session state from VirtualBox
 */
int VBoxInstance::loadSessions( const FiniteTaskPtr & pf ) {
    CRASH_REPORT_BEGIN;
    HVSessionPtr inst;
    vector<string> lines;
    map<string, string> vms, diskinfo, vboxVms;
    string secret, kk, kv;
    string err;

    // Acquire a system-wide mutex for session update
    NAMED_MUTEX_LOCK("session-update");

    // Initialize progress feedback
    if (pf) {
        pf->setMax(4);
        pf->doing("Loading sessions from disk");
    }

    // Reset sessions array
    if (!sessions.empty())
        sessions.clear();

    // [1] Load session registry from the disk
    // =======================================
    std::vector< std::string > vbDiskSessions  = LocalConfig::runtime()->enumFiles("vbsess-");
    for (std::vector< std::string >::iterator it = vbDiskSessions.begin(); it != vbDiskSessions.end(); ++it) {
        std::string sessName = *it;
        CVMWA_LOG("Debug", "Importing session config " << sessName << " from disk");

        // Load session config
        LocalConfigPtr sessConfig = LocalConfig::forRuntime( sessName );
        if (!sessConfig->contains("name")) {
            CVMWA_LOG("Warning", "Missing 'name' in file " << sessName );
        } else if (!sessConfig->contains("uuid")) {
            CVMWA_LOG("Warning", "Missing 'uuid' in file " << sessName );
        } else {
            // Store session with the given UUID
            sessions[ sessConfig->get("uuid") ] = boost::make_shared< VBoxSession >( 
                sessConfig, this->shared_from_this() 
            );
        }

    }

    // List the running VMs in the system
    int ans;
    ans = this->exec("list vms", &lines, &err, execConfig);
    if (ans != 0) return HVE_QUERY_ERROR;

    // Forward progress
    if (pf) {
        pf->done("Sessions loaded");
        pf->doing("Loading sessions from hypervisor");
    }

    // [2] Collect the running VM info
    // ================================
    vms = tokenize( &lines, '{' );
    for (std::map<string, string>::iterator it=vms.begin(); it!=vms.end(); ++it) {
        string name = (*it).first;
        string uuid = (*it).second;
        name = name.substr(1, name.length()-3);
        uuid = uuid.substr(0, uuid.length()-1);

        // Make sure it's not an inaccessible machine
        if (name.find("<inaccessible>") != string::npos) {
            CVMWA_LOG("Warning", "Found inaccessible VM " << uuid)
            continue;
        }

        // Store on map
        vboxVms[uuid] = name;
    }

    // Forward progress
    if (pf) {
        pf->done("Sessions loaded");
        pf->doing("Cleaning-up expired sessions");
    }

    // [3] Remove the VMs that are not registered 
    //     in the hypervisor.
    // ===========================================
    for (std::map< std::string,HVSessionPtr >::iterator it = this->sessions.begin(); it != this->sessions.end(); ++it) {
        HVSessionPtr sess = (*it).second;

        // Check if the stored session does not correlate
        // to a session in VirtualBox -> It means it was 
        // destroyed externally.
        if (vboxVms.find(sess->parameters->get("vboxid")) == vboxVms.end()) {

            // Make sure the session is not aware of that
            if (sess->state != STATE_CLOSED) {

                // Remove it from session map and rewind
                sessions.erase( sess->uuid );
                it = this->sessions.begin();

                // Quit if we are done
                if (this->sessions.size() == 0) break;

            }

        }

    }

    // Forward progress
    if (pf) {
        pf->done("Sessions cleaned-up");
        pf->doing("Releasing old open sessions");
    }

    // [4] Check if some of the currently open session 
    //     was lost.
    // ===========================================
    for (std::list< HVSessionPtr >::iterator it = openSessions.begin(); it != openSessions.end(); ++it) {
        HVSessionPtr sess = (*it);

        // Check if the session has gone away
        if (sessions.find(sess->uuid) == sessions.end()) {
   
            // Let session know that it has gone away
            boost::static_pointer_cast<VBoxSession>(sess)->hvNotifyDestroyed();

            // Remove it from open and rewind
            openSessions.erase( it );
            it = openSessions.begin();

            // Quit if we are done
            if (openSessions.size() == 0) break;

        }

    }

    // Notify progress
    if (pf) pf->done("Old open sessions released");

    return 0;
    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Abort what's happening and prepare for shutdown
 */
void VBoxInstance::abort() {

    // Abort all open sessions
    for (std::list< HVSessionPtr >::iterator it = openSessions.begin(); it != openSessions.end(); ++it) {
        HVSessionPtr sess = (*it);
        sess->abort();
    }

    // Cleanup
    openSessions.clear();
    sessions.clear();

}

/**
 * Check if the hypervisor has the extension pack installed (used for the more advanced RDP)
 */
bool VBoxInstance::hasExtPack() {
    CRASH_REPORT_BEGIN;
    
    /**
     * Check for extension pack
     */
    vector<string> lines;
    string err;
    NAMED_MUTEX_LOCK("generic");
    this->exec("list extpacks", &lines, &err, execConfig);
    NAMED_MUTEX_UNLOCK;
    for (std::vector<std::string>::iterator l = lines.begin(); l != lines.end(); l++) {
        if (l->find("Oracle VM VirtualBox Extension Pack") != string::npos) {
            return true;
        }
    }

    // Not found
    return false;
    CRASH_REPORT_END;
}

/**
 * Install extension pack
 *
 * This function is used in combination with the installHypervisor function from Hypervisor class, but it can also be used
 * on it's own.
 *
 */
int VBoxInstance::installExtPack( const DownloadProviderPtr & downloadProvider, const FiniteTaskPtr & pf ) {
    CRASH_REPORT_BEGIN;
    string requestBuf;
    string checksum;
    string err;

    // Local exec config
    SysExecConfig config(execConfig);

    // Notify extension pack installation
    if (pf) {
        pf->setMax(5, false);
        pf->doing("Preparing for extension pack installation");
    }

    // If we already have an extension pack, complete
    if (hasExtPack()) {
        if (pf) pf->complete("Already installed");
        return HVE_ALREADY_EXISTS;
    }

    // Begin a download task
    VariableTaskPtr downloadPf;
    if (pf) downloadPf = pf->begin<VariableTask>("Downloading hypervisor configuration");
    
    /* Contact the information point */
    CVMWA_LOG( "Info", "Fetching data" );
    int res = downloadProvider->downloadText( URL_HYPERVISOR_CONFIG FBSTRING_PLUGIN_VERSION, &requestBuf, downloadPf );
    if ( res != HVE_OK ) {
        if (pf) pf->fail("Unable to fetch hypervisor configuration", res);
        return res;
    }
    
    // Extract information
    vector<string> lines;
    splitLines( requestBuf, &lines );
    map<string, string> data = tokenize( &lines, '=' );

    // Build version string (it will be something like "vbox-2.4.12")
    ostringstream oss;
    oss << "vbox-" << version.major << "." << version.minor << "." << version.build;

    CVMWA_LOG("INFO", "Ver string: '" << oss.str() << "' from '" << version.verString << "'");

    // Prepare name constants to be looked up on the configuration url
    string kExtpackUrl = oss.str()      + "-extpack";
    string kExtpackChecksum = oss.str() + "-extpackChecksum";
    string kExtpackExt = ".vbox-extpack";

    // Verify integrity of the data
    if (data.find( kExtpackUrl ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No extensions package URL found" );
        if (pf) pf->fail("No extensions package URL found", HVE_EXTERNAL_ERROR);
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kExtpackChecksum ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No extensions package checksum found" );
        if (pf) pf->fail("No extensions package checksum found", HVE_EXTERNAL_ERROR);
        return HVE_EXTERNAL_ERROR;
    }

    // Begin download
    if (pf) downloadPf = pf->begin<VariableTask>("Downloading hypervisor configuration");

    // Download extension pack
    string tmpExtpackFile = getTmpDir() + "/" + getFilename( data[kExtpackUrl] );
    CVMWA_LOG( "Info", "Downloading " << data[kExtpackUrl] << " to " << tmpExtpackFile  );
    res = downloadProvider->downloadFile( data[kExtpackUrl], tmpExtpackFile, downloadPf );
    CVMWA_LOG( "Info", "    : Got " << res  );
    if ( res != HVE_OK ) {
        if (pf) pf->fail("Unable to download extension pack", res);
        return res;
    }
    
    // Validate checksum
    if (pf) pf->doing("Validating extension pack integrity");
    sha256_file( tmpExtpackFile, &checksum );
    CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data[kExtpackChecksum]  );
    if (checksum.compare( data[kExtpackChecksum] ) != 0) {
        if (pf) pf->fail("Extension pack integrity was not validated", HVE_NOT_VALIDATED);
        return HVE_NOT_VALIDATED;
    }
    if (pf) pf->done("Extension pack integrity validated");

    // Install extpack on virtualbox
    if (pf) pf->doing("Installing extension pack");
    NAMED_MUTEX_LOCK("generic");
    res = this->exec( "extpack install \"" + tmpExtpackFile + "\"", NULL, &err, config.setGUI(true) );
    NAMED_MUTEX_UNLOCK;
    if (res != HVE_OK) {
        if (pf) pf->fail("Extension pack failed to install", HVE_EXTERNAL_ERROR);
        return HVE_EXTERNAL_ERROR;
    }
    if (pf) pf->done("Installed extension pack");

    // Cleanup
    if (pf) pf->doing("Cleaning-up");
    remove( tmpExtpackFile.c_str() );
    if (pf) pf->done("Cleaned-up");

    // Complete
    if (pf) pf->complete("Extension pack installed successfully");
    return HVE_OK;

    CRASH_REPORT_END;
}
