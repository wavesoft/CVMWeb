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

#include "Hypervisor.h"
#include "Virtualbox.h"
#include "Utilities.h"

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
    map<string, string> info = this->getMachineInfo();
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
int VBoxSession::wrapExec( std::string cmd, std::vector<std::string> * stdoutList, std::string * stderrMsg, int retries ) {
    CRASH_REPORT_BEGIN;
    ostringstream oss;
    string line;
    string stderrLocal;
    int ans = 0;
    
    /* Debug log command */
    if (this->onDebug) (this->onDebug)("Executing '"+cmd+"'");
    
    /* Run command */
    ans = this->host->exec( cmd, stdoutList, &stderrLocal, this->m_ipcMutex, retries );
    
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

/**
 * Open new session
 */
int VBoxSession::open( int cpus, int memory, int disk, std::string cvmVersion, int flags ) { 
    CRASH_REPORT_BEGIN;
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string stdoutList, uuid, vmIso, kk, kv;
    int ans;
    bool needsUpdate;

    /* Validate state */
    if ((this->state != STATE_CLOSED) && (this->state != STATE_ERROR)) return HVE_INVALID_STATE;
    this->state = STATE_OPPENING;
    
    /* Update session */
    this->cpus = cpus;
    this->memory = memory;
    this->flags = flags;
        
    /* (1) Create slot */
    if (this->onProgress) (this->onProgress)(5, 110, "Allocating VM slot");
    ans = this->getMachineUUID( this->name, &uuid, flags );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return ans;
    } else {
        this->uuid = uuid;
        this->updateSharedMemoryID( uuid );
    }
    
    /* Find a random free port for VRDE */
    this->rdpPort = (rand() % 0xFBFF) + 1024;
    while (isPortOpen( "127.0.0.1", this->rdpPort ))
        this->rdpPort = (rand() % 0xFBFF) + 1024;

    /* Pick the boot medium depending on the mount type */
    string bootMedium = "dvd";
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) bootMedium = "disk";
    
    /* (2) Set parameters */
    args.str("");
    args << "modifyvm "
        << uuid
        << " --cpus "                   << cpus
        << " --memory "                 << memory
        << " --cpuexecutioncap "        << this->executionCap
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
    
    /* Enable graphical additions if instructed to do so */
    if ((flags & HVF_GRAPHICAL) != 0) {
        args
        << " --draganddrop "            << "hosttoguest"
        << " --clipboard "              << "bidirectional";
    }

    /* Check if we are NATing or if we are using the second NIC */
    if ((this->flags & HVF_DUAL_NIC) != 0) {

        /* =============================================================================== */
        /*   NETWORK MODE 1 : Dual Interface                                               */
        /* ------------------------------------------------------------------------------- */
        /*  In this mode a secondary, host-only adapter will be added to the VM, enabling  */
        /*  any kind of traffic to pass through to the VM. The API URL will be built upon  */
        /*  this IP address.                                                               */
        /* =============================================================================== */

        /* Detect the host-only adapter */
        if (this->onProgress) (this->onProgress)(10, 110, "Setting up local network");
        string ifHO = this->getHostOnlyAdapter();
        if (ifHO.empty()) {
            this->state = STATE_ERROR;
            return HVE_CREATE_ERROR;
        }

        /* Create a secondary adapter */
        args << " --nic2 "              << "hostonly" << " --hostonlyadapter2 \"" << ifHO << "\"";

    } else {

        /* =============================================================================== */
        /*   NETWORK MODE 2 : NAT on the main interface                                    */
        /* ------------------------------------------------------------------------------- */
        /*  In this mode a NAT port forwarding rule will be added to the first NIC that    */
        /*  enables communication only to the specified API port. This is much simpler     */
        /*  since the guest IP does not need to be known.                                  */
        /* =============================================================================== */

        /* Find a random free port for API */
        this->localApiPort = (rand() % 0xFBFF) + 1024;
        while (isPortOpen( "127.0.0.1", this->localApiPort ))
            this->localApiPort = (rand() % 0xFBFF) + 1024;

        /* Create a NAT rule to the API port */
        args << " --natpf1 "              << "guestapi,tcp,127.0.0.1," << this->localApiPort << ",," << this->apiPort;

    }

    /* Invoke the cmdline */
    if (this->onProgress) (this->onProgress)(15, 110, "Setting up VM");
    ans = this->wrapExec(args.str(), NULL);
    CVMWA_LOG( "Info", "Modify VM=" << ans  );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return HVE_MODIFY_ERROR;
    }

    /* Fetch information to validate disks */
    if (this->onProgress) (this->onProgress)(20, 110, "Fetching machine info");
    map<string, string> machineInfo = this->getMachineInfo();

    /* ============================================================================= */
    /*   MODE 1 : Regular Mode                                                       */
    /* ----------------------------------------------------------------------------- */
    /*   In this mode the 'cvmVersion' variable contains the URL of a gzipped VMDK   */
    /*   image. This image will be downloaded, extracted and placed on cache. Then   */
    /*   it will be cloned using copy-on write mode on the user's VM directory.      */
    /* ============================================================================= */
    
    if ((flags & HVF_DEPLOYMENT_HDD) != 0) {
        
        /**
         * Prepare download feedback
         */
        ProgressFeedback feedback;
        feedback.total = 110;
        feedback.min = 20;
        feedback.max = 90;
        feedback.callback = this->onProgress;
        feedback.message = "Downloading VM Disk";
        feedback.__lastEventTime = getMillis();
        
        /* (3) Download the disk image specified by the URL */
        string masterDisk;
        if (this->onProgress) (this->onProgress)(20, 110, "Downloading VM Disk");
        CVMWA_LOG("Info", "Downloading VM '" << cvmVersion << "' (SHA256=" << this->diskChecksum << ") to " << masterDisk);
        ans = this->host->diskImageDownload( cvmVersion, this->diskChecksum, &masterDisk, &feedback );
        if (ans < HVE_OK) {
            this->state = STATE_ERROR;
            return ans;
        }
        
        /* Store the source URL */
        this->setProperty("/CVMWeb/diskURL", cvmVersion);
        
        /* (4) Check if the VM actually has the image we need */
        needsUpdate = true;
        if (machineInfo.find( BOOT_DSK ) != machineInfo.end()) {
            
            /* Get the filename of the disk */
            getKV( machineInfo[ BOOT_DSK ], &kk, &kv, '(', 0 );
            kk = kk.substr(0, kk.length()-1);
            
            /* If they are the same, we are lucky */
            if (kk.compare( masterDisk ) == 0) {
                CVMWA_LOG( "Info", "Same disk (" << masterDisk << ")" );
                needsUpdate = false;
                
            } else {
                CVMWA_LOG( "Info", "Master disk is different : " << kk << " / " << masterDisk  );

                /* Unmount previount disk */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << BOOT_CONTROLLER
                    << " --port "       << BOOT_PORT
                    << " --device "     << BOOT_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress) (this->onProgress)(93, 110, "Detachining previous disk");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Detaching Disk=" << ans  );
                if (ans != 0) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
                
            }
            
        }
        
        /* If we must attach the disk, do it now */
        if (needsUpdate) {
            CVMWA_LOG("Info", "Disk needs to be updated");

            /* Prepare two locations where we can find the disk,
             * because before some version VirtualBox required the UUID if the master disk */
            string masterDiskPath = "\"" + masterDisk + "\"";
            string masterDiskUUID = "";
            
            /* Get a list of the disks in order to properly compute multi-attach */
            vector< map< string, string > > disks = this->host->getDiskList();
            for (vector< map<string, string> >::iterator i = disks.begin(); i != disks.end(); i++) {
                map<string, string> iface = *i;

                /* Look of the master disk of what we are using */
                if ( (iface.find("Type") != iface.end()) && (iface.find("Parent UUID") != iface.end()) && (iface.find("Location") != iface.end()) && (iface.find("UUID") != iface.end()) ) {

                    /* Check if all the component maches */
                    if ( (iface["Type"].compare("multiattach") == 0) && (iface["Parent UUID"].compare("base") == 0) && samePath(iface["Location"],masterDisk) ) {
                        
                        /* Use the master UUID instead of the filename */
                        CVMWA_LOG("Info", "Found master with UUID " << iface["UUID"]);
                        masterDiskUUID = "{" + iface["UUID"] + "}";
                        break;
                        
                    }
                }
            }
            
            /* (5a) Try to attach disk to the SATA controller using full path */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << BOOT_CONTROLLER
                << " --port "       << BOOT_PORT
                << " --device "     << BOOT_DEVICE
                << " --type "       << "hdd"
                << " --mtype "      << "multiattach"
                << " --medium "     <<  masterDiskPath;

            if (this->onProgress) (this->onProgress)(95, 110, "Attaching hard disk");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Storage Attach=" << ans  );
            if (ans == 0) {
                needsUpdate = false;
            } else {
                /* If we have no UUID available, we can't try the 5b */
                if (masterDiskUUID.empty()) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
            }
            
            /* (5b) Try to attach disk to the SATA controller using UUID (For older VirtualBox versions) */
            if (needsUpdate) {
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << BOOT_CONTROLLER
                    << " --port "       << BOOT_PORT
                    << " --device "     << BOOT_DEVICE
                    << " --type "       << "hdd"
                    << " --mtype "      << "multiattach"
                    << " --medium "     <<  masterDiskUUID;

                if (this->onProgress) (this->onProgress)(95, 110, "Attaching hard disk");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Storage Attach=" << ans  );
                if (ans != 0) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
            }
            
        }

        
    }

    /* ============================================================================= */
    /*   MODE 2 : CernVM-Micro Mode                                                  */
    /* ----------------------------------------------------------------------------- */
    /*   In this mode a new blank, scratch disk is created. The 'cvmVersion'         */
    /*   contains the version of the VM to be downloaded.                            */
    /* ============================================================================= */
    else {
        
        /* Check for scratch disk */
        if (machineInfo.find( SCRATCH_DSK ) == machineInfo.end()) {

            /* Create a hard disk for this VM */
            string vmDisk = getTmpFile(".vdi", this->getDataFolder());

            /* (4) Create disk */
            args.str("");
            args << "createhd"
                << " --filename "   << "\"" << vmDisk << "\""
                << " --size "       << disk;

            if (this->onProgress) (this->onProgress)(25, 110, "Creating scratch disk");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Create HD=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }

            /* (5) Attach disk to the SATA controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << SCRATCH_CONTROLLER
                << " --port "       << SCRATCH_PORT
                << " --device "     << SCRATCH_DEVICE
                << " --type "       << "hdd"
                << " --setuuid "    << "\"\"" 
                << " --medium "     << "\"" << vmDisk << "\"";

            if (this->onProgress) (this->onProgress)(35, 110, "Attaching hard disk");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Storage Attach=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }

        }

        /* Check if the CernVM Version the machine is using is the one we need */
        needsUpdate = true;
        if (machineInfo.find( BOOT_DSK ) != machineInfo.end()) {

            /* Get the filename of the iso */
            getKV( machineInfo[ BOOT_DSK ], &kk, &kv, '(', 0 );
            kk = kk.substr(0, kk.length()-1);

            /* Get the filename of the given version */
            this->host->cernVMCached( cvmVersion, &kv );

            /* If they are the same, we are lucky */
            if (kv.compare(kk) == 0) {
                CVMWA_LOG( "Info", "Same versions (" << kv << ")" );
                needsUpdate = false;
            } else {
                CVMWA_LOG( "Info", "CernVM iso is different : " << kk << " / " << kv  );

                /* Unmount previount iso */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << BOOT_CONTROLLER
                    << " --port "       << BOOT_PORT
                    << " --device "     << BOOT_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress) (this->onProgress)(40, 110, "Detachining previous CernVM ISO");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Detaching ISO=" << ans  );
                if (ans != 0) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
            }

        }

        /* Check if we need to update the CD-ROM attaching business */
        if (needsUpdate) {

            /**
             * Prepare download feedback
             */
            ProgressFeedback feedback;
            feedback.total = 110;
            feedback.min = 40;
            feedback.max = 90;
            feedback.callback = this->onProgress;
            feedback.message = "Downloading CernVM";

            /* Download CernVM */
            if (this->onProgress) (this->onProgress)(40, 110, "Downloading CernVM");
            if (this->host->cernVMDownload( cvmVersion, &vmIso, &feedback ) != 0) {
                this->state = STATE_ERROR;
                return HVE_IO_ERROR;
            }

            /* (6) Attach boot CD-ROM to the controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << BOOT_CONTROLLER
                << " --port "       << BOOT_PORT
                << " --device "     << BOOT_DEVICE
                << " --type "       << "dvddrive"
                << " --medium "     << "\"" << vmIso << "\"";

            if (this->onProgress) (this->onProgress)(95, 110, "Attaching CD-ROM");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Storage Attach (CernVM)=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }        
        }
        
    }
    
    /* Check if we should attach guest additions */
    #ifdef GUESTADD_USE
    if ( ((flags & HVF_GUEST_ADDITIONS) != 0) && !this->host->hvGuestAdditions.empty() ) {
        
        /* Check if they are already mounted the guest additions */
        needsUpdate = true;
        if (machineInfo.find( GUESTADD_DSK ) != machineInfo.end()) {

            /* Get the filename of the iso */
            getKV( machineInfo[ GUESTADD_DSK ], &kk, &kv, '(', 0 );
            kk = kk.substr(0, kk.length()-1);

            /* If they are the same, we are lucky */
            if (this->host->hvGuestAdditions.compare(kk) == 0) {
                CVMWA_LOG( "Info", "Same file (" << this->host->hvGuestAdditions << ")" );
                needsUpdate = false;
                
            } else {
                CVMWA_LOG( "Info", "Guest additions ISO is different : " << kk << " / " << this->host->hvGuestAdditions  );

                /* Unmount previount iso */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << GUESTADD_CONTROLLER
                    << " --port "       << GUESTADD_PORT
                    << " --device "     << GUESTADD_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress) (this->onProgress)(100, 110, "Detachining previous GuestAdditions ISO");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Detaching ISO=" << ans  );
                if (ans != 0) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
            }

        }

        /* Check if we need to update the CD-ROM attaching business */
        if (needsUpdate) {

            /* (6) Attach boot CD-ROM to the controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << GUESTADD_CONTROLLER
                << " --port "       << GUESTADD_PORT
                << " --device "     << GUESTADD_DEVICE
                << " --type "       << "dvddrive"
                << " --medium "     << "\"" << this->host->hvGuestAdditions << "\"";

            if (this->onProgress) (this->onProgress)(105, 110, "Attaching GuestAdditions CD-ROM");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Storage Attach (GuestAdditions)=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }
        }

    }
    #endif
    
    /* Store web-secret on the guest properties */
    this->setProperty("/CVMWeb/secret", this->key);
    this->setProperty("/CVMWeb/localApiPort", ntos<int>(this->localApiPort));
    this->setProperty("/CVMWeb/daemon/controlled", (this->daemonControlled ? "1" : "0"));
    this->setProperty("/CVMWeb/daemon/cap/min", ntos<int>(this->daemonMinCap));
    this->setProperty("/CVMWeb/daemon/cap/max", ntos<int>(this->daemonMaxCap));
    this->setProperty("/CVMWeb/daemon/flags", ntos<int>(this->daemonFlags));
    this->setProperty("/CVMWeb/userData", base64_encode(this->userData));

    /* Last callbacks */
    if (this->onProgress) (this->onProgress)(110, 110, "Completed");

    /* Notify OPEN state change */
    this->state = STATE_OPEN;
    if (this->onOpen) (this->onOpen)();
    
    return HVE_OK;
    CRASH_REPORT_END;
}

std::string macroReplace( std::map<std::string,std::string> *uData, std::string iString ) {
    CRASH_REPORT_BEGIN;
    size_t iPos, ePos, lPos = 0, tokStart = 0, tokLen = 0;
    while ( (iPos = iString.find("${", lPos)) != string::npos ) {

        // Find token bounds
        CVMWA_LOG("Debug", "Found '${' at " << iPos);
        tokStart = iPos;
        iPos += 2;
        ePos = iString.find("}", iPos);
        if (ePos == string::npos) break;
        CVMWA_LOG("Debug", "Found '}' at " << ePos);
        tokLen = ePos - tokStart;

        // Extract token value
        string token = iString.substr(tokStart+2, tokLen-2);
        CVMWA_LOG("Debug", "Token is '" << token << "'");
        
        // Extract default
        string vDefault = "";
        iPos = token.find(":");
        if (iPos != string::npos) {
            CVMWA_LOG("Debug", "Found ':' at " << iPos );
            token = token.substr(0, iPos);
            vDefault = token.substr(iPos+1);
            CVMWA_LOG("Debug", "Default is '" << vDefault << "', token is '" << token << "'" );
        }

        
        // Look for token value
        string vValue = vDefault;
        CVMWA_LOG("Debug", "Checking value" );
        if (uData != NULL)
            if (uData->find(token) != uData->end())
                vValue = uData->at(token);
        
        // Replace value
        CVMWA_LOG("Debug", "Value is '" << vValue << "'" );
        iString = iString.substr(0,tokStart) + vValue + iString.substr(tokStart+tokLen+1);
        
        // Move forward
        CVMWA_LOG("Debug", "String replaced" );
        lPos = tokStart + tokLen;
    }
    
    // Return replaced data
    return iString;
    CRASH_REPORT_END;
};

/**
 * Start VM with the given
 */
int VBoxSession::start( std::map<std::string,std::string> *uData ) { 
    CRASH_REPORT_BEGIN;
    string vmContextDsk, vmPatchedUserData, kk, kv;
    vmPatchedUserData = this->userData;
    ostringstream args;
    int ans;
    
    CVMWA_LOG("Debug", "userData: '" << vmPatchedUserData << "'");
    CVMWA_LOG("Debug", "uData==NULL : " << ((uData == NULL) ? "true" : "false") );
    if (uData != NULL) {
        CVMWA_LOG("Debug", "uData->empty() : " << (uData->empty() ? "true" : "false") );
    }
    CVMWA_LOG("Debug", "vmPatchedUserData.empty() : " << (vmPatchedUserData.empty() ? "true" : "false") );
    
    /* Update local userData */
    if ( !vmPatchedUserData.empty() ) {

        CVMWA_LOG("Debug", "Replacing from '" << vmPatchedUserData << "'");
        vmPatchedUserData = macroReplace( uData, vmPatchedUserData );
        CVMWA_LOG("Debug", "Replaced to '" << vmPatchedUserData << "'");

    }

    /* Validate state */
    CVMWA_LOG( "Info", "0 (" << this->state << ")" );
    if (this->state != STATE_OPEN) return HVE_INVALID_STATE;
    this->state = STATE_STARTING;

    /* Fetch information to validate disks */
    map<string, string> machineInfo = this->getMachineInfo();
    
    /* Check if vm is in saved state */
    bool inSavedState = false;
    if (machineInfo.find( "State" ) != machineInfo.end()) {
        if (machineInfo["State"].find("saved") != string::npos) {
            inSavedState = true;
        }
    }

    CVMWA_LOG("Debug", "inSavedState : " << (inSavedState ? "true" : "false") );
    
    /* Touch context ISO only if we have user-data and the VM is not hibernated */
    if (!vmPatchedUserData.empty() && !(uData == NULL) && !inSavedState) {
        CVMWA_LOG("Debug", "Going to attach User-Data with '" << vmPatchedUserData << "'");
        
        /* Check if we are using FloppyIO instead of contextualization CD */
        if ((this->flags & HVF_FLOPPY_IO) != 0) {
            
            /* ========================== */
            /*  CONTEXTUALIZATION FLOPPY  */
            /* ========================== */
            
            /* Detach & Delete previous context ISO */
            if (machineInfo.find( FLOPPYIO_DSK ) != machineInfo.end()) {
        
                /* Get the filename of the iso */
                getKV( machineInfo[ FLOPPYIO_DSK ], &kk, &kv, '(', 0 );
                kk = kk.substr(0, kk.length()-1);

                CVMWA_LOG( "Info", "Detaching " << kk  );

                /* Detach floppy */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << FLOPPYIO_CONTROLLER
                    << " --port "       << FLOPPYIO_PORT
                    << " --device "     << FLOPPYIO_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress) (this->onProgress)(1, 7, "Detaching configuration floppy");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Storage Attach (floppyIO)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Unregister/delete floppy */
                args.str("");
                args << "closemedium floppy "
                    << "\"" << kk << "\"";

                if (this->onProgress) (this->onProgress)(2, 7, "Closing configuration floppy");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Closemedium (floppyIO)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Delete actual file */
                if (this->onProgress) (this->onProgress)(3, 7, "Removing configuration floppy");
                remove( kk.c_str() );
        
            }
    
            /* Create Context floppy */
            if (this->onProgress) (this->onProgress)(4, 7, "Building configuration floppy");
            if (this->host->buildFloppyIO( vmPatchedUserData, &vmContextDsk ) != 0) 
                return HVE_CREATE_ERROR;

            /* Attach context Floppy to the floppy controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << FLOPPYIO_CONTROLLER
                << " --port "       << FLOPPYIO_PORT
                << " --device "     << FLOPPYIO_DEVICE
                << " --type "       << "fdd"
                << " --medium "     << "\"" << vmContextDsk << "\"";

            if (this->onProgress) (this->onProgress)(5, 7, "Attaching configuration floppy");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "StorageAttach (floppyIO)=" << ans  );
            if (ans != 0) {
                this->state = STATE_OPEN;
                return HVE_MODIFY_ERROR;
            }
            
        } else {

            /* ========================== */
            /*  CONTEXTUALIZATION CD-ROM  */
            /* ========================== */

            /* Detach & Delete previous context ISO */
            if (machineInfo.find( CONTEXT_DSK ) != machineInfo.end()) {
        
                /* Get the filename of the iso */
                getKV( machineInfo[ CONTEXT_DSK ], &kk, &kv, '(', 0 );
                kk = kk.substr(0, kk.length()-1);

                CVMWA_LOG( "Info", "Detaching " << kk  );

                /* Detach iso */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << CONTEXT_CONTROLLER
                    << " --port "       << CONTEXT_PORT
                    << " --device "     << CONTEXT_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress) (this->onProgress)(1, 7, "Detaching contextualization CD-ROM");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Storage Attach (context)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Unregister/delete iso */
                args.str("");
                args << "closemedium dvd "
                    << "\"" << kk << "\"";

                if (this->onProgress) (this->onProgress)(2, 7, "Closing contextualization CD-ROM");
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Closemedium (context)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Delete actual file */
                if (this->onProgress) (this->onProgress)(3, 7, "Removing contextualization CD-ROM");
                remove( kk.c_str() );
        
            }
    
            /* Create Context ISO */
            if (this->onProgress) (this->onProgress)(4, 7, "Building contextualization CD-ROM");
            if (this->host->buildContextISO( vmPatchedUserData, &vmContextDsk ) != 0) 
                return HVE_CREATE_ERROR;

            /* Attach context CD-ROM to the IDE controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << CONTEXT_CONTROLLER
                << " --port "       << CONTEXT_PORT
                << " --device "     << CONTEXT_DEVICE
                << " --type "       << "dvddrive"
                << " --medium "     << "\"" << vmContextDsk << "\"";

            if (this->onProgress) (this->onProgress)(5, 7, "Attaching contextualization CD-ROM");
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "StorageAttach (context)=" << ans  );
            if (ans != 0) {
                this->state = STATE_OPEN;
                return HVE_MODIFY_ERROR;
            }
            
        }
    }
    
    /* Start VM */
    if (this->onProgress) (this->onProgress)(6, 7, "Starting VM");
    if ((this->flags & HVF_HEADFUL) != 0) {
        ans = this->wrapExec("startvm " + this->uuid + " --type gui", NULL, NULL, 4);
    } else {
        ans = this->wrapExec("startvm " + this->uuid + " --type headless", NULL, NULL, 4);
    }
    CVMWA_LOG( "Info", "Start VM=" << ans  );
    if (ans != 0) {
        this->state = STATE_OPEN;
        return HVE_MODIFY_ERROR;
    }
    
    /* Update parameters */
    if (this->onProgress) (this->onProgress)(7, 7, "Completed");
    this->state = STATE_STARTED;
    
    /* We don't know the IP address of the VM. During it's possible hibernation
       state the DHCP lease might have been released. */
    this->ip = "";
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return 0;
    CRASH_REPORT_END;
}

/**
 * Close VM
 */
int VBoxSession::close( bool unmonitored ) { 
    CRASH_REPORT_BEGIN;
    string kk, kv;
    ostringstream args;
    int ans;

    /* If we are running unmonitored, all the wrapExec()
       calls have negative value, which means that they
       will run in the background. */
    int retries = 4;
    if (unmonitored) retries = -1;

    /* Validate state */
    if ((this->state != STATE_OPEN) && 
        (this->state != STATE_STARTED) && 
        (this->state != STATE_PAUSED) &&
        (this->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Stop the VM if it's running (we don't care about the warnings) */
    if (this->onProgress) (this->onProgress)(1, 10, "Shutting down the VM");
    this->controlVM( "poweroff");
    
    /* Unmount, release and delete media */
    map<string, string> machineInfo = this->getMachineInfo();
    
    /* Check if vm is in saved state */
    if (machineInfo.find( "State" ) != machineInfo.end()) {
        if (machineInfo["State"].find("saved") != string::npos) {
            
            if (this->onProgress) (this->onProgress)(2, 10, "Discarding saved VM state");
            ans = this->wrapExec("discardstate " + this->uuid, NULL, NULL, retries);
            CVMWA_LOG( "Info", "Discarded VM state=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_CONTROL_ERROR;
            }
            
        }
    }
    
    /* Detach & Delete context ISO */
    if (machineInfo.find( CONTEXT_DSK ) != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo[ CONTEXT_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        CVMWA_LOG( "Info", "Detaching " << kk  );

        /* Detach iso */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << CONTEXT_CONTROLLER
            << " --port "       << CONTEXT_PORT
            << " --device "     << CONTEXT_DEVICE
            << " --medium "     << "none";

        if (this->onProgress) (this->onProgress)(3, 10, "Detaching contextualization CD-ROM");
        ans = this->wrapExec(args.str(), NULL, NULL, retries);
        CVMWA_LOG( "Info", "Storage Attach (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium dvd "
            << "\"" << kk << "\"";

        if (this->onProgress) (this->onProgress)(4, 10, "Closing contextualization CD-ROM");
        ans = this->wrapExec(args.str(), NULL, NULL, retries);
        CVMWA_LOG( "Info", "Closemedium (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress) (this->onProgress)(5, 10, "Deleting contextualization CD-ROM");
        remove( kk.c_str() );
        
    }
    
    /* Detach & Delete Disk */
    if (machineInfo.find( SCRATCH_DSK ) != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo[ SCRATCH_DSK ], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        CVMWA_LOG( "Info", "Detaching " << kk  );

        /* Detach iso */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << SCRATCH_CONTROLLER
            << " --port "       << SCRATCH_PORT
            << " --device "     << SCRATCH_DEVICE
            << " --medium "     << "none";

        if (this->onProgress) (this->onProgress)(6, 10, "Detaching data disk");
        ans = this->wrapExec(args.str(), NULL, NULL, retries);
        CVMWA_LOG( "Info", "Storage Attach (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium disk "
            << "\"" << kk << "\"";

        if (this->onProgress) (this->onProgress)(7, 10, "Closing data disk medium");
        ans = this->wrapExec(args.str(), NULL, NULL, retries);
        CVMWA_LOG( "Info", "Closemedium (disk)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress) (this->onProgress)(8, 10, "Removing data disk");
        remove( kk.c_str() );
        
    }    
    
    /* Unregister and delete VM */
    if (this->onProgress) (this->onProgress)(9, 10, "Deleting VM");
    ans = this->wrapExec("unregistervm " + this->uuid + " --delete", NULL, NULL, retries);
    CVMWA_LOG( "Info", "Unregister VM=" << ans  );
    /* We don't care for errors here */
    
    /* OK */
    if (this->onProgress) (this->onProgress)(10, 10, "Completed");
    this->state = STATE_CLOSED;
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Create or fetch the UUID of the VM with the given name
 */
int VBoxSession::getMachineUUID( std::string mname, std::string * ans_uuid, int flags ) {
    CRASH_REPORT_BEGIN;
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string secret, name, uuid;
    
    /* List the running VMs in the system */
    int ans = this->wrapExec("list vms", &lines);
    if (ans != 0) {
        return HVE_QUERY_ERROR;
    }
    
    /* Tokenize */
    toks = tokenize( &lines, '{' );
    for (std::map<string, string>::iterator it=toks.begin(); it!=toks.end(); ++it) {
        name = (*it).first;
        uuid = (*it).second;
        name = name.substr(1, name.length()-3);
        uuid = uuid.substr(0, uuid.length()-1);
        
        /* Compare name */
        if (name.compare(mname) == 0)  {
            *ans_uuid = "{" + uuid + "}";
            return 0;
        }
    }
    
    /* Check what kind of linux to create */
    string osType = "Linux26";
    if ((flags & HVF_SYSTEM_64BIT) != 0) osType="Linux26_64";
    
    /* Not found, create VM */
    args.str("");
    args << "createvm"
        << " --name \"" << mname << "\""
        << " --ostype " << osType
        << " --register";
    
    ans = this->wrapExec(args.str(), &lines);
    if (ans != 0) return HVE_CREATE_ERROR;
    
    /* Parse output */
    toks = tokenize( &lines, ':' );
    uuid = toks["UUID"];
    
    /* (3a) Attach an IDE controller */
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "IDE"
        << " --add "        << "ide";
    
    ans = this->wrapExec(args.str(), NULL);
    if (ans != 0) return HVE_MODIFY_ERROR;

    /* (3b) Attach a SATA controller */
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "SATA"
        << " --add "        << "sata";
    
    ans = this->wrapExec(args.str(), NULL);
    if (ans != 0) return HVE_MODIFY_ERROR;
    
    /* (3c) If we are using floppyIO, include a floppy controller */
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << FLOPPYIO_CONTROLLER
        << " --add "        << "floppy";

    ans = this->wrapExec(args.str(), NULL);
    if (ans != 0) return HVE_MODIFY_ERROR;
    
    /* OK */
    *ans_uuid = "{" + uuid + "}";
    return 0;
    CRASH_REPORT_END;
}

/**
 * Resume VM
 */
int VBoxSession::resume() {
    CRASH_REPORT_BEGIN;
    int ans;
    
    /* Validate state */
    if (this->state != STATE_PAUSED) return HVE_INVALID_STATE;
    
    /* We don't know the IP address of the VM. During it's pause
       state the DHCP lease might have been released. */
    this->ip = "";
    
    /* Resume VM */
    ans = this->controlVM("resume");
    this->state = STATE_STARTED;
    return ans;
    CRASH_REPORT_END;
}

/**
 * Pause VM
 */
int VBoxSession::pause() {
    CRASH_REPORT_BEGIN;
    int ans; 
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Pause VM */
    ans = this->controlVM("pause");
    this->state = STATE_PAUSED;
    return ans;
    CRASH_REPORT_END;
}

/**
 * Reset VM
 */
int VBoxSession::reset() {
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Reset VM */
    return this->controlVM( "reset" );
    CRASH_REPORT_END;
}

/**
 * Stop VM
 */
int VBoxSession::stop() {
    CRASH_REPORT_BEGIN;
    int ans;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Stop VM */
    ans = this->controlVM( "poweroff" );
    this->state = STATE_OPEN;
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return ans;
    
    CRASH_REPORT_END;
}

/**
 * Stop VM
 */
int VBoxSession::hibernate() {
    CRASH_REPORT_BEGIN;
    int ans;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Stop VM */
    ans = this->controlVM( "savestate" );
    this->state = STATE_OPEN;
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();

    return ans;
    CRASH_REPORT_END;
}

/**
 * Set execution cap
 */
int VBoxSession::setExecutionCap(int cap) {
    CRASH_REPORT_BEGIN;
    ostringstream os;
    int ans;

    /* If it's not started, use modifyVM */
    if ((this->state == STATE_OPEN) || (this->state == STATE_ERROR)) {
        
        // VM Inactive -> Modify
        os << "modifyvm "
            << this->uuid
            << " --cpuexecutioncap " << cap;
        ans = this->wrapExec( os.str(), NULL);
        if (ans == 0) {
            this->executionCap = cap;
            return cap;
        } else {
            return ans; /* Failed */
        }

    } else {
        
        /* Reject invalid values of cap */
        if ((cap <=0) || (cap > 100))
            return HVE_USAGE_ERROR;
        
        // VM Active -> Control
        os << "cpuexecutioncap " << cap;
        ans = this->controlVM( os.str());
        if (ans == 0) {
            this->executionCap = cap;
            return cap;
        } else {
            return ans; /* Failed */
        }

    }

    CRASH_REPORT_END;
}

/**
 * Ensure the existance and return the name of the host-only adapter in the system
 */
std::string VBoxSession::getHostOnlyAdapter() {
    CRASH_REPORT_BEGIN;

    vector<string> lines;
    vector< map<string, string> > ifs;
    vector< map<string, string> > dhcps;
    string ifName = "", vboxName, ipServer, ipMin, ipMax;
    
    /* Check if we already have host-only interfaces */
    int ans = this->wrapExec("list hostonlyifs", &lines);
    if (ans != 0) return "";
    
    /* Check if there is really nothing */
    if (lines.size() == 0) {
        ans = this->wrapExec("hostonlyif create", NULL, NULL, 2);
        if (ans != 0) return "";
    
        /* Repeat check */
        ans = this->wrapExec("list hostonlyifs", &lines, NULL, 2);
        if (ans != 0) return "";
        
        /* Still couldn't pick anything? Error! */
        if (lines.size() == 0) return "";
    }
    ifs = tokenizeList( &lines, ':' );
    
    /* Dump the DHCP server states */
    ans = this->wrapExec("list dhcpservers", &lines);
    if (ans != 0) return "";
    dhcps = tokenizeList( &lines, ':' );
    
    /* The name of the first network found and a flag
       to check if we were able to find a DHCP server */
    bool    foundDHCPServer = false;
    string  foundIface      = "",
            foundBaseIP     = "",
            foundVBoxName   = "",
            foundMask       = "";

    /* Process interfaces */
    for (vector< map<string, string> >::iterator i = ifs.begin(); i != ifs.end(); i++) {
        map<string, string> iface = *i;

        CVMWA_LOG("log", "Checking interface");
        mapDump(iface);

        /* Ensure proper environment */
        if (iface.find("Name") == iface.end()) continue;
        if (iface.find("VBoxNetworkName") == iface.end()) continue;
        if (iface.find("IPAddress") == iface.end()) continue;
        if (iface.find("NetworkMask") == iface.end()) continue;
        
        /* Fetch interface info */
        ifName = iface["Name"];
        vboxName = iface["VBoxNetworkName"];
        
        /* Check if we have DHCP enabled on this interface */
        bool hasDHCP = false;
        for (vector< map<string, string> >::iterator i = dhcps.begin(); i != dhcps.end(); i++) {
            map<string, string> dhcp = *i;
            if (dhcp.find("NetworkName") == dhcp.end()) continue;
            if (dhcp.find("Enabled") == dhcp.end()) continue;

            CVMWA_LOG("log", "Checking dhcp");
            mapDump(dhcp);
            
            /* The network has a DHCP server, check if it's running */
            if (vboxName.compare(dhcp["NetworkName"]) == 0) {
                if (dhcp["Enabled"].compare("Yes") == 0) {
                    hasDHCP = true;
                    break;
                    
                } else {
                    
                    /* Make sure the server does not have an invalid IP address */
                    bool updateIPInfo = false;
                    if (dhcp["IP"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["lowerIPAddress"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["upperIPAddress"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (dhcp["NetworkMask"].compare("0.0.0.0") == 0) updateIPInfo=true;
                    if (updateIPInfo) {
                        
                        /* Prepare IP addresses */
                        ipServer = changeUpperIP( iface["IPAddress"], 100 );
                        ipMin = changeUpperIP( iface["IPAddress"], 101 );
                        ipMax = changeUpperIP( iface["IPAddress"], 254 );
                    
                        /* Modify server */
                        ans = this->wrapExec(
                            "dhcpserver modify --ifname \"" + ifName + "\"" +
                            " --ip " + ipServer +
                            " --netmask " + iface["NetworkMask"] +
                            " --lowerip " + ipMin +
                            " --upperip " + ipMax
                             , NULL, NULL, 2);
                        if (ans != 0) continue;
                    
                    }
                    
                    /* Check if we can enable the server */
                    ans = this->wrapExec("dhcpserver modify --ifname \"" + ifName + "\" --enable", NULL);
                    if (ans == 0) {
                        hasDHCP = true;
                        break;
                    }
                    
                }
            }
        }
        
        /* Keep the information of the first interface found */
        if (foundIface.empty()) {
            foundIface = ifName;
            foundVBoxName = vboxName;
            foundBaseIP = iface["IPAddress"];
            foundMask = iface["NetworkMask"];
        }
        
        /* If we found DHCP we are done */
        if (hasDHCP) {
            foundDHCPServer = true;
            break;
        }
        
    }
    
    /* If there was no DHCP server, create one */
    if (!foundDHCPServer) {
        
        /* Prepare IP addresses */
        ipServer = changeUpperIP( foundBaseIP, 100 );
        ipMin = changeUpperIP( foundBaseIP, 101 );
        ipMax = changeUpperIP( foundBaseIP, 254 );
        
        /* Add and start server */
        ans = this->wrapExec(
            "dhcpserver add --ifname \"" + foundIface + "\"" +
            " --ip " + ipServer +
            " --netmask " + foundMask +
            " --lowerip " + ipMin +
            " --upperip " + ipMax +
            " --enable"
             , NULL);
        if (ans != 0) return "";
                
    }
    
    /* Got my interface */
    return foundIface;
    CRASH_REPORT_END;
};

/**
 * Return a property from the VirtualBox guest
 */
std::string VBoxSession::getProperty( std::string name ) { 
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    string value;
    
    /* Invoke property query */
    int ans = this->wrapExec("guestproperty get "+this->uuid+" \""+name+"\"", &lines);
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
 * Set a property to the VirtualBox guest
 */
int VBoxSession::setProperty( std::string name, std::string value ) { 
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    
    /* Perform property update */
    int ans = this->wrapExec("guestproperty set "+this->uuid+" \""+name+"\" \""+value+"\"", &lines);
    if (ans != 0) return HVE_MODIFY_ERROR;
    return 0;
    
    CRASH_REPORT_END;
}

/**
 * Send a controlVM something
 */
int VBoxSession::controlVM( std::string how ) {
    CRASH_REPORT_BEGIN;
    int ans = this->wrapExec("controlvm "+this->uuid+" "+how, NULL, NULL, 4);
    if (ans != 0) return HVE_CONTROL_ERROR;
    return 0;
    CRASH_REPORT_END;
}

/**
 * Start the Virtual Machine
 */
int VBoxSession::startVM() {
    CRASH_REPORT_BEGIN;
    int ans = this->wrapExec("startvm "+this->uuid+" --type headless", NULL, NULL, 4);
    if (ans != 0) return HVE_CONTROL_ERROR;

    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return 0;
    CRASH_REPORT_END;
}

/** 
 * Return virtual machine information
 */
map<string, string> VBoxSession::getMachineInfo() {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->wrapExec("showvminfo "+this->uuid, &lines);
    if (ans != 0) {
        dat[":ERROR:"] = ntos<int>( ans );
        return dat;
    }
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
    CRASH_REPORT_END;
};

/** 
 * Return the RDP Connection host
 */
std::string VBoxSession::getRDPHost() {
    CRASH_REPORT_BEGIN;
    char numstr[21]; // enough to hold all numbers up to 64-bits
    sprintf(numstr, "%d", this->rdpPort);
    std::string ip = "127.0.0.1:";
    return ip + numstr;
    CRASH_REPORT_END;
}

/**
 * Update session information
 */
int VBoxSession::update() {
    CRASH_REPORT_BEGIN;
    return this->host->updateSession( this, false );
    CRASH_REPORT_END;
}

/**
 * Update session information (Fast version)
 */
int VBoxSession::updateFast() {
    CRASH_REPORT_BEGIN;
    return this->host->updateSession( this, true );
    CRASH_REPORT_END;
}

/**
 * Return the IP Address of the session
 */
std::string VBoxSession::getAPIHost() {
    CRASH_REPORT_BEGIN;

    // The API host is different in various NIC modes
    if ((this->flags & HVF_DUAL_NIC) != 0) {

        // (1) The API Host is on the second interface
        if (this->ip.empty()) {
            std::string guestIP = this->getProperty("/VirtualBox/GuestInfo/Net/1/V4/IP");
            if (!guestIP.empty()) {
                this->ip = guestIP;
                return guestIP;
            } else {
                return "";
            }
        } else {
            return this->ip;
        }

    } else {

        // (2) The API Host is on localhost
        return "127.0.0.1";

    }
    CRASH_REPORT_END;
}

/**
 * Return the actual API Port where to contact
 */
int VBoxSession::getAPIPort() {
    CRASH_REPORT_BEGIN;

    // The API port is different in various NIC modes
    if ((this->flags & HVF_DUAL_NIC) != 0) {

        // (1) The API Port is the actual API port in the guest.
        return this->apiPort;

    } else {

        // (2) The API Port is a random port on the host, which
        //     is forwarded via NAT in the guest.
        return this->localApiPort;

    }

    CRASH_REPORT_END;
}

/**
 * Return miscelaneous information
 */
std::string VBoxSession::getExtraInfo( int extraInfo ) {
    CRASH_REPORT_BEGIN;
    if (extraInfo == EXIF_VIDEO_MODE) {
        map<string, string> info = this->getMachineInfo();
        if (info.find("Video mode") != info.end())
            return info["Video mode"];
    }

    return "";
    CRASH_REPORT_END;
}

/** =========================================== **\
            Virtualbox Implementation
\** =========================================== **/

/** 
 * Return virtual machine information
 */
map<string, string> Virtualbox::getMachineInfo( std::string uuid ) {
    CRASH_REPORT_BEGIN;
    vector<string> lines;
    map<string, string> dat;
    string err;
    
    /* Perform property update */
    int ans = this->exec("showvminfo "+uuid, &lines, &err, NULL, 4 );
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
    if (this->exec( "guestproperty enumerate "+uuid, &lines, &errOut, NULL, 4 ) == 0) {
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
    int ans = this->exec("guestproperty get "+uuid+" \""+name+"\"", &lines, &err, NULL, 2);
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
    int ans = this->exec("list hostcpuids", &lines, &err, NULL, 2);
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
    ans = this->exec("list systemproperties", &lines, &err, NULL, 2);
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
    int ans = this->exec("list hdds", &lines, &err, NULL, 2);
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
    ifstream fIn(logFile, ifstream::in);
    
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
    
    /* Get session's uuid */
    string uuid = session->uuid;
    if (uuid.empty()) return HVE_USAGE_ERROR;
    
    /* Collect details */
    map<string, string> info = this->getMachineInfo( uuid );
    if (info.empty()) 
        return HVE_NOT_FOUND;
    if (info.find(":ERROR:") != info.end())
        return HVE_IO_ERROR;
    
    /* Reset flags */
    session->flags = 0;
    
    /* Check state */
    session->state = STATE_OPEN;
    if (info.find("State") != info.end()) {
        string state = info["State"];
        if (state.find("running") != string::npos) {
            session->state = STATE_STARTED;
        } else if (state.find("paused") != string::npos) {
            session->state = STATE_PAUSED;
        } else if (state.find("saved") != string::npos) {
            session->state = STATE_OPEN;
        } else if (state.find("aborted") != string::npos) {
            session->state = STATE_OPEN;
        }
    }
    
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
            } else {
                pStart += 6;
                size_t pEnd = rdpInfo.find(",", pStart);
                string rdpPort = rdpInfo.substr( pStart, pEnd-pStart );

                // Apply the rdp port
                ((VBoxSession *)session)->rdpPort = ston<int>(rdpPort);
            }
        } else {
            ((VBoxSession *)session)->rdpPort = 0;
        }
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
        int ans = this->exec("showhdinfo \""+kk+"\"", &lines, &err, NULL, 2);
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

        /* Get hypervisor pid from file */
        if (info.find("Log folder") != info.end())
            session->pid = __getPIDFromFile( info["Log folder"] );

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
    int ans = this->exec("list vms", &lines, &err, NULL, 2);
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
            session->updateSharedMemoryID( session->uuid );

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
    this->exec("list extpacks", &lines, &err, NULL, 2);
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
    res = this->exec("extpack install \"" + tmpExtpackFile + "\"", NULL, &err, NULL, 2);
    if (res != HVE_OK) return HVE_EXTERNAL_ERROR;

    /* Cleanup */
    if (cbProgress) (cbProgress)(progressMax, progressTotal, "Cleaning up extension");
  	remove( tmpExtpackFile.c_str() );
    return HVE_OK;

    CRASH_REPORT_END;
}
