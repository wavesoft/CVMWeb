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

#include "VBoxHypervisor.h"
#include "Common/Hypervisor.h"
#include "Common/Utilities.h"

using namespace std;

// Where to mount the bootable CD-ROM
#define BOOT_CONTROLLER     "IDE"
#define BOOT_PORT           "0"
#define BOOT_DEVICE         "0"

// Where to mount the scratch disk
#define SCRATCH_CONTROLLER  "SATA"
#define SCRATCH_PORT        "0"
#define SCRATCH_DEVICE      "0"

// Where to mount the contextualization CD-ROM
#define CONTEXT_CONTROLLER  "SATA"
#define CONTEXT_PORT        "1"
#define CONTEXT_DEVICE      "0"

// Where (if to) mount the guest additions CD-ROM
#define GUESTADD_USE        1
#define GUESTADD_CONTROLLER "SATA"
#define GUESTADD_PORT       "2"
#define GUESTADD_DEVICE     "0"

// Where the floppyIO floppy is placed
#define FLOPPYIO_ENUM_NAME  "Storage Controller Name (2)"
    // ^^ The first controller is IDE, second is SATA, third is Floppy
#define FLOPPYIO_CONTROLLER "Floppy"
#define FLOPPYIO_PORT       "0"
#define FLOPPYIO_DEVICE     "0"

/** =========================================== **\
                   Tool Functions
\** =========================================== **/

// Create some condensed strings using the above parameters
#define BOOT_DSK            BOOT_CONTROLLER " (" BOOT_PORT ", " BOOT_DEVICE ")"
#define SCRATCH_DSK         SCRATCH_CONTROLLER " (" SCRATCH_PORT ", " SCRATCH_DEVICE ")"
#define CONTEXT_DSK         CONTEXT_CONTROLLER " (" CONTEXT_PORT ", " CONTEXT_DEVICE ")"
#define GUESTADD_DSK        GUESTADD_CONTROLLER " (" GUESTADD_PORT ", " GUESTADD_DEVICE ")"
#define FLOPPYIO_DSK        FLOPPYIO_CONTROLLER " (" FLOPPYIO_PORT ", " FLOPPYIO_DEVICE ")"

/**
 * Extract the mac address of the VM from the NIC line definition
 */
std::string extractMac( std::string nicInfo ) {
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
 * Replace the last part of the given IP
 */
std::string changeUpperIP( std::string baseIP, int value ) {
    CRASH_REPORT_BEGIN;
    size_t iDot = baseIP.find_last_of(".");
    if (iDot == string::npos) return "";
    return baseIP.substr(0, iDot) + "." + ntos<int>(value);
    CRASH_REPORT_END;
};


/** =========================================== **\
            Virtualbox Implementation
\** =========================================== **/

/** 
 * Return virtual machine information
 */
map<string, string> Virtualbox::getMachineInfo( std::string uuid, int timeout ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;
    string err;
    
    /* Perform property update */
    int ans;
    NAMED_MUTEX_LOCK( uuid );
    ans = this->exec("showvminfo "+uuid, &lines, &err, 4, timeout );
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
map<string, string> Virtualbox::getAllProperties( string uuid ) {
    CRASH_REPORT_BEGIN;
    map<string, string> ans;
    vector<string> lines;
    string errOut;

    /* Get guest properties */
    NAMED_MUTEX_LOCK( uuid );
    if (this->exec( "guestproperty enumerate "+uuid, &lines, &errOut, 4, 2000 ) == 0) {
        for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++) {
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
bool Virtualbox::waitTillReady( std::string pluginVersion, callbackProgress cbProgress, int progressMin , int progressMax, int progressTotal ) {
    CRASH_REPORT_BEGIN;
    
    /**
     * Session loading takes time, so instead of blocking the plugin
     * at creation time, use this mechanism to delay-load it when first accessed.
     */
    if (!this->sessionLoaded) {
        this->loadSessions();
        this->sessionLoaded = true;
    }
    
    /**
     * By the way, check if we have the extension pack installed
     */
    if (!this->hasExtPack()) {
        this->installExtPack(
            pluginVersion,
            this->downloadProvider,
            cbProgress, progressMin, progressMax, progressTotal
            );
    }

    /**
     * All's good!
     */
    return true;
    CRASH_REPORT_END;
}

/**
 * Return a property from the VirtualBox guest
 */
std::string Virtualbox::getProperty( std::string uuid, std::string name ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    string value;
    string err;
    
    /* Invoke property query */
    int ans;
    NAMED_MUTEX_LOCK( uuid );
    ans = this->exec("guestproperty get "+uuid+" \""+name+"\"", &lines, &err, 2, 2000);
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
HVSession * Virtualbox::allocateSession( std::string name, std::string key ) {
    CRASH_REPORT_BEGIN;
    VBoxSession * sess = new VBoxSession();
    sess->name = name;
    sess->key = key;
    sess->host = this;
    sess->rdpPort = 0;
    sess->localApiPort = 0;
    sess->dataPath = "";
    sess->properties.clear();
    sess->unsyncedProperties.clear();
    sess->updateLock = false;
    return sess;
    CRASH_REPORT_END;
}

/**
 * Load capabilities
 */
int Virtualbox::getCapabilities ( HVINFO_CAPS * caps ) {
    CRASH_REPORT_BEGIN;
    map<string, string> data;
    vector<string> lines, parts;
    string err;
    int v;
    
    /* List the CPUID information */
    int ans;
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list hostcpuids", &lines, &err, 2);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return HVE_QUERY_ERROR;
    if (lines.empty()) return HVE_EXTERNAL_ERROR;
    
    /* Process lines */
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
    
    /* Update flags */
    caps->cpu.hasVM = false; // Needs MSR to detect
    caps->cpu.hasVT = 
        ( (caps->cpu.featuresA & 0x20) != 0 ) || // Intel 'vmx'
        ( (caps->cpu.featuresC & 0x2)  != 0 );   // AMD 'svm'
    caps->cpu.has64bit =
        ( (caps->cpu.featuresC & 0x20000000) != 0 ); // Long mode 'lm'
        
    /* List the system properties */
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list systemproperties", &lines, &err, 2);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return HVE_QUERY_ERROR;
    if (lines.empty()) return HVE_EXTERNAL_ERROR;

    /* Default limits */
    caps->max.cpus = 1;
    caps->max.memory = 1024;
    caps->max.disk = 2048;
    
    /* Tokenize into the data map */
    parseLines( &lines, &data, ":", " \t", 0, 1 );
    if (data.find("Maximum guest RAM size") != data.end()) 
        caps->max.memory = ston<int>(data["Maximum guest RAM size"]);
    if (data.find("Virtual disk limit (info)") != data.end()) 
        caps->max.disk = ston<long>(data["Virtual disk limit (info)"]) / 1024;
    if (data.find("Maximum guest CPU count") != data.end()) 
        caps->max.cpus = ston<int>(data["Maximum guest CPU count"]);
    
    /* Ok! */
    return HVE_OK;
    CRASH_REPORT_END;
};

/**
 * Get a list of mediums managed by VirtualBox
 */
std::vector< std::map< std::string, std::string > > Virtualbox::getDiskList() {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    std::vector< std::map< std::string, std::string > > resMap;
    string err;

    /* List the running VMs in the system */
    int ans;
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list hdds", &lines, &err, 2, 2000);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return resMap;
    if (lines.empty()) return resMap;

    /* Tokenize lists */
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

/**
 * Update session information from VirtualBox
 */
int Virtualbox::updateSession( HVSession * session, bool fast ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    string err;
    bool prevEditable;

    /* Don't update session if we are in middle of something */
    if ((session->state == STATE_STARTING) || (session->state == STATE_OPPENING) || ((VBoxSession*)session)->updateLock)
        return HVE_INVALID_STATE;
    
    /* Get session's uuid */
    string uuid = session->uuid;
    if (uuid.empty()) return HVE_USAGE_ERROR;
    
    /* Collect details */
    map<string, string> info = this->getMachineInfo( uuid, 2000 );
    if (info.empty()) 
        return HVE_NOT_FOUND;
    if (info.find(":ERROR:") != info.end())
        return HVE_IO_ERROR;
    
    /* Reset flags */
    prevEditable = session->editable;
    session->flags = 0;
    session->editable = true;
    
    /* Check state */
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
    
    /* If session switched to editable state, commit pending property changes */
    if (session->editable && !prevEditable && !((VBoxSession*)session)->unsyncedProperties.empty()) {
        for (std::map<string, string>::iterator it=((VBoxSession*)session)->unsyncedProperties.begin(); 
                it!=((VBoxSession*)session)->unsyncedProperties.end(); ++it) {
            string name = (*it).first;
            string value = (*it).second;
            
            // Session is now editable
            session->setProperty(name, value);
        }
    }
    
    /* Reset property maps */
    ((VBoxSession*)session)->unsyncedProperties.clear();
    ((VBoxSession*)session)->properties.clear();
    
    /* Get CPU */
    if (info.find("Number of CPUs") != info.end()) {
        session->cpus = ston<int>( info["Number of CPUs"] );
    }
    
    /* Check flags */
    if (info.find("Guest OS") != info.end()) {
        string guestOS = info["Guest OS"];
        if (guestOS.find("64 bit") != string::npos) {
            session->flags |= HVF_SYSTEM_64BIT;
        }
    }

    /* Find configuration folder */
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

    /* Parse RDP info */
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
    
    /* Parse memory */
    if (info.find("Memory size") != info.end()) {
        session->cpus = ston<int>( info["Number of CPUs"] );

        /* Parse memory */
        string mem = info["Memory size"];
        mem = mem.substr(0, mem.length()-2);
        session->memory = ston<int>(mem);

    }
    
    /* Parse CernVM Version from the ISO */
    session->version = DEFAULT_CERNVM_VERSION;
    if (info.find( BOOT_DSK ) != info.end()) {

        /* Get the filename of the iso */
        getKV( info[ BOOT_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        /* Extract CernVM Version from file */
        session->version = this->cernVMVersion( kk );
        if (session->version.empty()) { 
            // (If we could not find a version number it's a disk-deployment)
            session->version = getFilename( kk );
            session->flags |= HVF_DEPLOYMENT_HDD;
        }
    }
    
    /* Check if there are guest additions mounted and update flags */
    if (info.find( GUESTADD_DSK ) != info.end()) {
        
        /* Get the filename of the iso */
        getKV( info[ GUESTADD_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        /* Check if we have the guest additions disk */
        if ( kk.compare(this->hvGuestAdditions) == 0 ) 
            session->flags |= HVF_GUEST_ADDITIONS;
        
    }
    
    /* Check if we have floppy adapter. If we do, it means we are using floppyIO -> updateFlags */
    if (info.find( FLOPPYIO_ENUM_NAME ) != info.end()) {
        session->flags |= HVF_FLOPPY_IO;
    }
    
    /* Parse disk size */
    session->disk = 1024;
    if (info.find( SCRATCH_DSK ) != info.end()) {

        /* Get the filename of the iso */
        getKV( info[ SCRATCH_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        /* Collect disk info */
        int ans;
        NAMED_MUTEX_LOCK(kk);
        ans = this->exec("showhdinfo \""+kk+"\"", &lines, &err, 2, 2000);
        NAMED_MUTEX_UNLOCK;
        if (ans == 0) {
        
            /* Tokenize data */
            diskinfo = tokenize( &lines, ':' );
            if (diskinfo.find("Logical size") != diskinfo.end()) {
                kk = diskinfo["Logical size"];
                kk = kk.substr(0, kk.length()-7); // Strip " MBytes"
                session->disk = ston<int>(kk);
            }
            
        }
    }
    
    /* Parse execution cap */
    session->executionCap = 100;
    if (info.find( "CPU exec cap" ) != info.end()) {
        
        /* Get execution cap */
        kk = info["CPU exec cap"];
        kk = kk.substr(0, kk.length()-1); // Strip %
        
        /* Convert to number */
        session->executionCap = ston<int>( kk );
        
    }
    
    /* If we want to be fast, skip time-consuming operations */
    if (!fast) {

        /* Parse all properties concurrently */
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
            session->userData = "";
        } else {
            session->userData = base64_decode(allProps["/CVMWeb/userData"]);
        }

        if (allProps.find("/CVMWeb/localApiPort") == allProps.end()) {
            ((VBoxSession *)session)->localApiPort = 0;
        } else {
            ((VBoxSession *)session)->localApiPort = ston<int>(allProps["/CVMWeb/localApiPort"]);
        }
        CVMWA_LOG("Debug", "LocalAPI Port = " << ((VBoxSession *)session)->localApiPort);

        /* Get hypervisor pid from file */
        if (info.find("Log folder") != info.end())
            session->pid = __getPIDFromFile( info["Log folder"] );
        
        /* Store allProps to properties */
        ((VBoxSession*)session)->properties = allProps;

    }

    /* Updated successfuly */
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Load session state from VirtualBox
 */
int Virtualbox::loadSessions() {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    string err;
    
    /* List the running VMs in the system */
    int ans;
    NAMED_MUTEX_LOCK("generic");
    ans = this->exec("list vms", &lines, &err, 2, 2000);
    NAMED_MUTEX_UNLOCK;
    if (ans != 0) return HVE_QUERY_ERROR;

    /* Tokenize */
    vms = tokenize( &lines, '{' );
    this->sessions.clear();
    for (std::map<string, string>::iterator it=vms.begin(); it!=vms.end(); ++it) {
        string name = (*it).first;
        string uuid = (*it).second;
        name = name.substr(1, name.length()-3);
        uuid = uuid.substr(0, uuid.length()-1);
        
        /* Check if this VM has a secret web key. If yes, it's managed by the WebAPI */
        secret = this->getProperty( uuid, "/CVMWeb/secret" );
        if (!secret.empty()) {
            
            /* Create a populate session object */
            HVSession * session = this->allocateSession( name, secret );
            session->uuid = "{" + uuid + "}";
            session->key = secret;

            /* Update session info */
            updateSession( session, false );

            /* Register this session */
            CVMWA_LOG( "Info", "Registering session name=" << session->name << ", key=" << session->key << ", uuid=" << session->uuid << ", state=" << session->state  );
            this->registerSession(session);

        }
    }

    return 0;
    CRASH_REPORT_END;
}

/**
 * Check if the hypervisor has the extension pack installed (used for the more advanced RDP)
 */
bool Virtualbox::hasExtPack() {
    CRASH_REPORT_BEGIN;
    
    /**
     * Check for extension pack
     */
    vector<string> lines;
    string err;
    NAMED_MUTEX_LOCK("generic");
    this->exec("list extpacks", &lines, &err, 2, 2000);
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
int Virtualbox::installExtPack( string versionID, DownloadProviderPtr downloadProvider, callbackProgress cbProgress, int progressMin, int progressMax, int progressTotal ) {
    CRASH_REPORT_BEGIN;

    /* Notify extension pack installation */
    int currProgress = progressMin;
    if (cbProgress) (cbProgress)(currProgress, progressTotal, "Downloading extension pack configuration");
    
    /* Contact the information point */
    string requestBuf;
    string checksum;
    string err;
    CVMWA_LOG( "Info", "Fetching data" );
    int res = downloadProvider->downloadText( "http://cernvm.cern.ch/releases/webapi/hypervisor.config?ver=" + versionID, &requestBuf );
    if ( res != HVE_OK ) return res;
    
    /* Extract information */
    vector<string> lines;
    splitLines( requestBuf, &lines );
    map<string, string> data = tokenize( &lines, '=' );
    
    /* Get the version of Virtualbox currently installed */
    size_t verPart = this->verString.find(" ");
    if (verPart == string::npos) verPart = this->verString.length();
    size_t revPart = this->verString.find("r");
    if (revPart == string::npos) revPart = verPart;

    /* Build version string (it will be something like "vbox-2.4.12") */
    string verstring = "vbox-" + this->verString.substr(0, revPart);

    /* Prepare name constants to be looked up on the configuration url */
    string kExtpackUrl = verstring      + "-extpack";
    string kExtpackChecksum = verstring + "-extpackChecksum";
    string kExtpackExt = ".vbox-extpack";

    /* Verify integrity of the data */
    if (data.find( kExtpackUrl ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No extensions package URL found" );
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kExtpackChecksum ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No extensions package checksum found" );
        return HVE_EXTERNAL_ERROR;
    }

    /* Divide progress in 5 steps */
    int progressStep = (int)( ((float) progressMax - (float)progressMin) / 5.0f );

    /* Update progres */
    ProgressFeedback feedback;
    feedback.total = progressTotal;
    feedback.min = currProgress;
    feedback.max = currProgress + 2*progressStep;
    feedback.callback = cbProgress;
    feedback.message = "Downloading extension pack";

    /* Download extension pack */
    string tmpExtpackFile = getTmpDir() + "/" + getFilename( data[kExtpackUrl] );
    if (cbProgress) (cbProgress)(currProgress, progressTotal, "Downloading extension pack");
    CVMWA_LOG( "Info", "Downloading " << data[kExtpackUrl] << " to " << tmpExtpackFile  );
    res = downloadProvider->downloadFile( data[kExtpackUrl], tmpExtpackFile, &feedback );
    CVMWA_LOG( "Info", "    : Got " << res  );
    if ( res != HVE_OK ) return res;
    
    /* Validate checksum */
    currProgress += 3*progressStep;
    if (cbProgress) (cbProgress)(currProgress, progressTotal, "Validating extension integrity");
    sha256_file( tmpExtpackFile, &checksum );
    CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data[kExtpackChecksum]  );
    if (checksum.compare( data[kExtpackChecksum] ) != 0) return HVE_NOT_VALIDATED;

    /* Install extpack on virtualbox */
    currProgress += progressStep;
    if (cbProgress) (cbProgress)(currProgress, progressTotal, "Installing extension pack");
    NAMED_MUTEX_LOCK("generic");
    res = this->exec("extpack install \"" + tmpExtpackFile + "\"", NULL, &err, 2);
    NAMED_MUTEX_UNLOCK;
    if (res != HVE_OK) return HVE_EXTERNAL_ERROR;

    /* Cleanup */
    if (cbProgress) (cbProgress)(progressMax, progressTotal, "Cleaning up extension");
    remove( tmpExtpackFile.c_str() );
    return HVE_OK;

    CRASH_REPORT_END;
}


/** =========================================== **\
            VBoxSession Implementation
\** =========================================== **/

/**
 * Return the location of the folder where we can create disks and other
 * non-volatile files.
 */
std::string VBoxSession::getDataFolder() {
    CRASH_REPORT_BEGIN;

    // If we already have a path, return it
    if (!this->dataPath.empty())
        return this->dataPath;

    // Get machine info
    map<string, string> info = this->getMachineInfo( 2000 );
    if (info.empty()) 
        return "";

    // Find configuration folder
    if (info.find("Config file") != info.end()) {
        string settingsFolder = info["Config file"];

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
 * Execute command and log debug message
 */
int VBoxSession::wrapExec( std::string cmd, std::vector<std::string> * stdoutList, std::string * stderrMsg, int retries, int timeout ) {
    CRASH_REPORT_BEGIN;
    ostringstream oss;
    string line;
    string stderrLocal;
    int ans = 0;
    
    /* Debug log command */
    if (this->onDebug) (this->onDebug)("Executing '"+cmd+"'");
    
    /* Run command */
    NAMED_MUTEX_LOCK( this->uuid );
    ans = this->host->exec( cmd, stdoutList, &stderrLocal, retries, timeout );
    NAMED_MUTEX_UNLOCK;
    
    /* Debug log response */
    if (this->onDebug) {

        // Parsed STDOUT
        if (stdoutList != NULL) {
            for (vector<string>::iterator i = stdoutList->begin(); i != stdoutList->end(); i++) {
                line = *i;
                (this->onDebug)("Line: "+line);
            }
        } else {
            (this->onDebug)("(Output ignored)");
        }

        // Raw STDERR
        if (!stderrLocal.empty())
            (this->onDebug)("Error: " + stderrLocal);

        // Exit code
        oss << "return = " << ans;
        (this->onDebug)(oss.str());
    }

    /* Forward stderr */
    if (stderrMsg != NULL) *stderrMsg = stderrLocal;
    
    /* Return exit code */
    return ans;
    CRASH_REPORT_END;

};
