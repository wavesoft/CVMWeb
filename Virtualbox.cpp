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
};

/**
 * Replace the last part of the given IP
 */
std::string changeUpperIP( std::string baseIP, int value ) {
    int iDot = baseIP.find_last_of(".");
    if (iDot == string::npos) return "";
    return baseIP.substr(0, iDot) + "." + ntos<int>(value);
};


/** =========================================== **\
            VBoxSession Implementation
\** =========================================== **/

/**
 * Execute command and log debug message
 */
int VBoxSession::wrapExec( std::string cmd, std::vector<std::string> * stdoutList ) {
    ostringstream oss;
    string line;
    int ans;
    
    /* Debug log command */
    if (this->onDebug!=NULL) (this->onDebug)("Executing '"+cmd+"'", this->cbObject);
    
    /* Run command */
    ans = this->host->exec( cmd, stdoutList );
    
    /* Debug log response */
    if (this->onDebug!=NULL) {
        if (stdoutList != NULL) {
            for (vector<string>::iterator i = stdoutList->begin(); i != stdoutList->end(); i++) {
                line = *i;
                (this->onDebug)("Line: "+line, this->cbObject);
            }
        } else {
            (this->onDebug)("(Output ignored)", this->cbObject);
        }
        oss << "return = " << ans;
        (this->onDebug)(oss.str(), this->cbObject);
    }
    return ans;
};

/**
 * Open new session
 */
int VBoxSession::open( int cpus, int memory, int disk, std::string cvmVersion, int flags ) { 
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string stdoutList, uuid, ifHO, vmIso, kk, kv;
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
    if (this->onProgress!=NULL) (this->onProgress)(5, 110, "Allocating VM slot", this->cbObject);
    ans = this->getMachineUUID( this->name, &uuid, flags );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return ans;
    } else {
        this->uuid = uuid;
    }
    
    /* Detect the host-only adapter */
    if (this->onProgress!=NULL) (this->onProgress)(10, 110, "Setting up local network", this->cbObject);
    ifHO = this->getHostOnlyAdapter();
    if (ifHO.empty()) {
        this->state = STATE_ERROR;
        return HVE_CREATE_ERROR;
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
        << " --natdnshostresolver1 "    << "on"
        << " --nic2 "                   << "hostonly" << " --hostonlyadapter2 \"" << ifHO << "\"";
    
    if (this->onProgress!=NULL) (this->onProgress)(15, 110, "Setting up VM", this->cbObject);
    ans = this->wrapExec(args.str(), NULL);
    CVMWA_LOG( "Info", "Modify VM=" << ans  );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return HVE_MODIFY_ERROR;
    }

    /* Fetch information to validate disks */
    if (this->onProgress!=NULL) (this->onProgress)(20, 110, "Fetching machine info", this->cbObject);
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
        HVPROGRESS_FEEDBACK feedback;
        feedback.total = 110;
        feedback.min = 20;
        feedback.max = 90;
        feedback.callback = this->onProgress;
        feedback.data = this->cbObject;
        feedback.message = "Downloading VM Disk";
        
        /* (3) Download the disk image specified by the URL */
        string masterDisk;
        if (this->onProgress!=NULL) (this->onProgress)(20, 110, "Downloading VM Disk", this->cbObject);
        ans = this->host->diskImageDownload( cvmVersion, &masterDisk, &feedback );
        if (ans < HVE_OK) {
            this->state = STATE_ERROR;
            return HVE_IO_ERROR;
        }
        
        /* Store the source URL */
        this->setProperty("/CVMWeb/diskURL", cvmVersion);
        
        /* (4) Check if the VM actually has the image we need */
        needsUpdate = true;
        if (machineInfo.find( BOOT_DSK ) != machineInfo.end()) {
            
            /* Get the filename of the iso */
            getKV( machineInfo[ BOOT_DSK ], &kk, &kv, '(', 0 );
            kk = kk.substr(0, kk.length()-1);
            
            /* If they are the same, we are lucky */
            if (kk.compare( masterDisk ) == 0) {
                CVMWA_LOG( "Info", "Same disk (" << masterDisk << ")" );
                needsUpdate = false;
                
            } else {
                CVMWA_LOG( "Info", "Master disk is different : " << kk << " / " << masterDisk  );

                /* Unmount previount iso */
                args.str("");
                args << "storageattach "
                    << uuid
                    << " --storagectl " << BOOT_CONTROLLER
                    << " --port "       << BOOT_PORT
                    << " --device "     << BOOT_DEVICE
                    << " --medium "     << "none";

                if (this->onProgress!=NULL) (this->onProgress)(93, 110, "Detachining previous disk", this->cbObject);
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Detaching ISO=" << ans  );
                if (ans != 0) {
                    this->state = STATE_ERROR;
                    return HVE_MODIFY_ERROR;
                }
                
            }
            
        }
        
        /* If we must attach the disk, do it now */
        if (needsUpdate) {
            
            /* (5) Attach disk to the SATA controller */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << BOOT_CONTROLLER
                << " --port "       << BOOT_PORT
                << " --device "     << BOOT_DEVICE
                << " --type "       << "hdd"
                << " --mtype "      << "multiattach"
                << " --medium "     << "\"" << masterDisk << "\"";

            if (this->onProgress!=NULL) (this->onProgress)(95, 110, "Attaching hard disk", this->cbObject);
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "Storage Attach=" << ans  );
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
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
            string vmDisk = getTmpFile(".vdi");

            /* (4) Create disk */
            args.str("");
            args << "createhd"
                << " --filename "   << "\"" << vmDisk << "\""
                << " --size "       << disk;

            if (this->onProgress!=NULL) (this->onProgress)(25, 110, "Creating scratch disk", this->cbObject);
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

            if (this->onProgress!=NULL) (this->onProgress)(35, 110, "Attaching hard disk", this->cbObject);
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

                if (this->onProgress!=NULL) (this->onProgress)(40, 110, "Detachining previous CernVM ISO", this->cbObject);
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
            HVPROGRESS_FEEDBACK feedback;
            feedback.total = 110;
            feedback.min = 40;
            feedback.max = 90;
            feedback.callback = this->onProgress;
            feedback.data = this->cbObject;
            feedback.message = "Downloading CernVM";

            /* Download CernVM */
            if (this->onProgress!=NULL) (this->onProgress)(40, 110, "Downloading CernVM", this->cbObject);
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

            if (this->onProgress!=NULL) (this->onProgress)(95, 110, "Attaching CD-ROM", this->cbObject);
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

                if (this->onProgress!=NULL) (this->onProgress)(100, 110, "Detachining previous GuestAdditions ISO", this->cbObject);
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

            if (this->onProgress!=NULL) (this->onProgress)(105, 110, "Attaching GuestAdditions CD-ROM", this->cbObject);
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
    this->setProperty("/CVMWeb/daemon/controlled", (this->daemonControlled ? "1" : "0"));
    this->setProperty("/CVMWeb/daemon/cap/min", ntos<int>(this->daemonMinCap));
    this->setProperty("/CVMWeb/daemon/cap/max", ntos<int>(this->daemonMaxCap));
    this->setProperty("/CVMWeb/daemon/flags", ntos<int>(this->daemonFlags));

    /* Last callbacks */
    if (this->onProgress!=NULL) (this->onProgress)(110, 110, "Completed", this->cbObject);

    /* Notify OPEN state change */
    this->state = STATE_OPEN;
    if (this->onOpen!=NULL) (this->onOpen)(this->cbObject);
    
    return HVE_OK;

}

/**
 * Start VM with the given
 */
int VBoxSession::start( std::string uData ) { 
    string vmContextDsk, kk, kv;
    ostringstream args;
    int ans;
    
    /* Update local userData */
    if (uData.compare("*") != 0) 
        userData = uData;

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
    
    /* Touch context ISO only if we have user-data and the VM is not hibernated */
    if (!userData.empty() && !inSavedState) {
        
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

                if (this->onProgress!=NULL) (this->onProgress)(1, 7, "Detaching configuration floppy", this->cbObject);
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

                if (this->onProgress!=NULL) (this->onProgress)(2, 7, "Closing configuration floppy", this->cbObject);
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Closemedium (floppyIO)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Delete actual file */
                if (this->onProgress!=NULL) (this->onProgress)(3, 7, "Removing configuration floppy", this->cbObject);
                remove( kk.c_str() );
        
            }
    
            /* Create Context floppy */
            if (this->onProgress!=NULL) (this->onProgress)(4, 7, "Building configuration floppy", this->cbObject);
            if (this->host->buildFloppyIO( userData, &vmContextDsk ) != 0) 
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

            if (this->onProgress!=NULL) (this->onProgress)(5, 7, "Attaching configuration floppy", this->cbObject);
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

                if (this->onProgress!=NULL) (this->onProgress)(1, 7, "Detaching contextualization CD-ROM", this->cbObject);
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

                if (this->onProgress!=NULL) (this->onProgress)(2, 7, "Closing contextualization CD-ROM", this->cbObject);
                ans = this->wrapExec(args.str(), NULL);
                CVMWA_LOG( "Info", "Closemedium (context)=" << ans  );
                if (ans != 0) {
                    this->state = STATE_OPEN;
                    return HVE_MODIFY_ERROR;
                }
        
                /* Delete actual file */
                if (this->onProgress!=NULL) (this->onProgress)(3, 7, "Removing contextualization CD-ROM", this->cbObject);
                remove( kk.c_str() );
        
            }
    
            /* Create Context ISO */
            if (this->onProgress!=NULL) (this->onProgress)(4, 7, "Building contextualization CD-ROM", this->cbObject);
            if (this->host->buildContextISO( userData, &vmContextDsk ) != 0) 
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

            if (this->onProgress!=NULL) (this->onProgress)(5, 7, "Attaching contextualization CD-ROM", this->cbObject);
            ans = this->wrapExec(args.str(), NULL);
            CVMWA_LOG( "Info", "StorageAttach (context)=" << ans  );
            if (ans != 0) {
                this->state = STATE_OPEN;
                return HVE_MODIFY_ERROR;
            }
            
        }
    }
    
    /* Start VM */
    if (this->onProgress!=NULL) (this->onProgress)(6, 7, "Starting VM", this->cbObject);
    ans = this->wrapExec("startvm " + this->uuid + " --type headless", NULL);
    CVMWA_LOG( "Info", "Start VM=" << ans  );
    if (ans != 0) {
        this->state = STATE_OPEN;
        return HVE_MODIFY_ERROR;
    }
    
    /* Update parameters */
    if (this->onProgress!=NULL) (this->onProgress)(7, 7, "Completed", this->cbObject);
    this->executionCap = 100;
    this->state = STATE_STARTED;
    
    /* Store user-data to the properties */
    if (userData.compare("*") != 0) this->setProperty("/CVMWeb/userData", base64_encode(userData));
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return 0;
}

/**
 * Close VM
 */
int VBoxSession::close() { 
    string kk, kv;
    ostringstream args;
    int ans;

    /* Validate state */
    if ((this->state != STATE_OPEN) && 
        (this->state != STATE_STARTED) && 
        (this->state != STATE_PAUSED) &&
        (this->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Stop the VM if it's running (we don't care about the warnings) */
    if (this->onProgress!=NULL) (this->onProgress)(1, 10, "Shutting down the VM", this->cbObject);
    this->controlVM( "poweroff");
    
    /* Unmount, release and delete media */
    map<string, string> machineInfo = this->getMachineInfo();
    
    /* Check if vm is in saved state */
    if (machineInfo.find( "State" ) != machineInfo.end()) {
        if (machineInfo["State"].find("saved") != string::npos) {
            
            if (this->onProgress!=NULL) (this->onProgress)(2, 10, "Discarding saved VM state", this->cbObject);
            ans = this->wrapExec("discardstate " + this->uuid, NULL);
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

        if (this->onProgress!=NULL) (this->onProgress)(3, 10, "Detaching contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        CVMWA_LOG( "Info", "Storage Attach (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium dvd "
            << "\"" << kk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(4, 10, "Closing contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        CVMWA_LOG( "Info", "Closemedium (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress!=NULL) (this->onProgress)(5, 10, "Deleting contextualization CD-ROM", this->cbObject);
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

        if (this->onProgress!=NULL) (this->onProgress)(6, 10, "Detaching data disk", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        CVMWA_LOG( "Info", "Storage Attach (context)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium disk "
            << "\"" << kk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(7, 10, "Closing data disk medium", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        CVMWA_LOG( "Info", "Closemedium (disk)=" << ans  );
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress!=NULL) (this->onProgress)(8, 10, "Removing data disk", this->cbObject);
        remove( kk.c_str() );
        
    }    
    
    /* Unregister and delete VM */
    if (this->onProgress!=NULL) (this->onProgress)(9, 10, "Deleting VM", this->cbObject);
    ans = this->wrapExec("unregistervm " + this->uuid + " --delete", NULL);
    CVMWA_LOG( "Info", "Unregister VM=" << ans  );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return HVE_CONTROL_ERROR;
    }
    
    /* OK */
    if (this->onProgress!=NULL) (this->onProgress)(10, 10, "Completed", this->cbObject);
    this->state = STATE_CLOSED;
    return HVE_OK;
}

/**
 * Create or fetch the UUID of the VM with the given name
 */
int VBoxSession::getMachineUUID( std::string mname, std::string * ans_uuid, int flags ) {
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
    if ((flags & HVF_FLOPPY_IO) != 0) {
        args.str("");
        args << "storagectl "
            << uuid
            << " --name "       << FLOPPYIO_CONTROLLER
            << " --add "        << "floppy";

        ans = this->wrapExec(args.str(), NULL);
        if (ans != 0) return HVE_MODIFY_ERROR;
    }
    
    /* OK */
    *ans_uuid = "{" + uuid + "}";
    return 0;
}

/**
 * Resume VM
 */
int VBoxSession::resume() {
    int ans;
    
    /* Validate state */
    if (this->state != STATE_PAUSED) return HVE_INVALID_STATE;
    
    /* Resume VM */
    ans = this->controlVM("resume");
    this->state = STATE_STARTED;
    return ans;
}

/**
 * Pause VM
 */
int VBoxSession::pause() {
    int ans; 
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Pause VM */
    ans = this->controlVM("pause");
    this->state = STATE_PAUSED;
    return ans;
}

/**
 * Reset VM
 */
int VBoxSession::reset() {
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Reset VM */
    return this->controlVM( "reset" );
}

/**
 * Stop VM
 */
int VBoxSession::stop() {
    int ans;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Stop VM */
    ans = this->controlVM( "poweroff" );
    this->state = STATE_OPEN;
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return ans;
    
}

/**
 * Stop VM
 */
int VBoxSession::hibernate() {
    int ans;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Stop VM */
    ans = this->controlVM( "savestate" );
    this->state = STATE_OPEN;
    
    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return ans;
    
}

/**
 * Set execution cap
 */
int VBoxSession::setExecutionCap(int cap) {
    ostringstream os;
    int ans;
    
    /* Validate state */
    if ((this->state != STATE_STARTED) && (this->state != STATE_OPEN)) return HVE_INVALID_STATE;

    os << "cpuexecutioncap " << cap;
    ans = this->controlVM( os.str());
    if (ans == 0) {
        this->executionCap = cap;
        return cap;
    } else {
        return ans; /* Failed */
    }
}

/**
 * Ensure the existance and return the name of the host-only adapter in the system
 */
std::string VBoxSession::getHostOnlyAdapter() {

    vector<string> lines;
    vector< map<string, string> > ifs;
    vector< map<string, string> > dhcps;
    string ifName = "", vboxName;
    
    /* Check if we already have host-only interfaces */
    int ans = this->wrapExec("list hostonlyifs", &lines);
    if (ans != 0) return "";
    
    /* Check if there is really nothing */
    if (lines.size() == 0) {
        ans = this->wrapExec("hostonlyif create", NULL);
        if (ans != 0) return "";
    
        /* Repeat check */
        ans = this->wrapExec("list hostonlyifs", &lines);
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
    bool    foundDHCPServer = false,
            foundDisabledDHCP = false;
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
                    
                    /* Check if we can enable the server */
                    ans = this->wrapExec("dhcpserver modify --ifname " + ifName + " --enable", NULL);
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
        string ipServer = changeUpperIP( foundBaseIP, 100 );
        string ipMin = changeUpperIP( foundBaseIP, 101 );
        string ipMax = changeUpperIP( foundBaseIP, 254 );
        
        /* Add and start server */
        ans = this->wrapExec(
            "dhcpserver add --ifname " + foundIface + 
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
};

/**
 * Return a property from the VirtualBox guest
 */
std::string VBoxSession::getProperty( std::string name ) { 
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
    
}

/**
 * Set a property to the VirtualBox guest
 */
int VBoxSession::setProperty( std::string name, std::string value ) { 
    vector<string> lines;
    
    /* Perform property update */
    int ans = this->wrapExec("guestproperty set "+this->uuid+" \""+name+"\" \""+value+"\"", &lines);
    if (ans != 0) return HVE_MODIFY_ERROR;
    return 0;
    
}

/**
 * Send a controlVM something
 */
int VBoxSession::controlVM( std::string how ) {
    int ans = this->wrapExec("controlvm "+this->uuid+" "+how, NULL);
    if (ans != 0) return HVE_CONTROL_ERROR;
    return 0;
}

/**
 * Start the Virtual Machine
 */
int VBoxSession::startVM() {
    int ans = this->wrapExec("startvm "+this->uuid+" --type headless", NULL);
    if (ans != 0) return HVE_CONTROL_ERROR;

    /* Check for daemon need */
    this->host->checkDaemonNeed();
    return 0;
}

/** 
 * Return virtual machine information
 */
map<string, string> VBoxSession::getMachineInfo() {
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->wrapExec("showvminfo "+this->uuid, &lines);
    if (ans != 0) return dat;
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
};

/** 
 * Return the RDP Connection host
 */
std::string VBoxSession::getRDPHost() {
    char numstr[21]; // enough to hold all numbers up to 64-bits
    sprintf(numstr, "%d", this->rdpPort);
    std::string ip = "127.0.0.1:";
    return ip + numstr;
}

/**
 * Update session information
 */
int VBoxSession::update() {
    return this->host->updateSession( this );
}

/** =========================================== **\
            Virtualbox Implementation
\** =========================================== **/

/** 
 * Return virtual machine information
 */
map<string, string> Virtualbox::getMachineInfo( std::string uuid ) {
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->exec("showvminfo "+uuid, &lines);
    if (ans != 0) return dat;
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
};


/**
 * Return a property from the VirtualBox guest
 */
std::string Virtualbox::getProperty( std::string uuid, std::string name ) {
    vector<string> lines;
    string value;
    
    /* Invoke property query */
    int ans = this->exec("guestproperty get "+uuid+" \""+name+"\"", &lines);
    if (ans != 0) return "";
    if (lines.empty()) return "";
    
    /* Process response */
    value = lines[0];
    if (value.substr(0,6).compare("Value:") == 0) {
        return value.substr(7);
    } else {
        return "";
    }
    
}

/**
 * Return Virtualbox sessions instead of classic
 */
HVSession * Virtualbox::allocateSession( std::string name, std::string key ) {
    VBoxSession * sess = new VBoxSession();
    sess->name = name;
    sess->key = key;
    sess->host = this;
    sess->rdpPort = 0;
    return sess;
}

/**
 * Load capabilities
 */
int Virtualbox::getCapabilities ( HVINFO_CAPS * caps ) {
    map<string, string> data;
    vector<string> lines, parts;
    int v;
    
    /* List the CPUID information */
    int ans = this->exec("list hostcpuids", &lines);
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
    ans = this->exec("list systemproperties", &lines);
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
    
};

/**
 * Get a list of mediums managed by VirtualBox
 */
std::vector< std::map< std::string, std::string > > Virtualbox::loadDisks() {
    vector<string> lines;
    std::vector< std::map< std::string, std::string > > resMap;

    /* List the running VMs in the system */
    int ans = this->exec("list hdds", &lines);
    if (ans != 0) return resMap;
    if (lines.empty()) return resMap;

    /* Tokenize lists */
    resMap = tokenizeList( &lines, ':' );
    return resMap;
}

/**
 * Update session information from VirtualBox
 */
int Virtualbox::updateSession( HVSession * session ) {
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    
    /* Get session's uuid */
    string uuid = session->uuid;
    if (uuid.empty()) return HVE_USAGE_ERROR;
    
    /* Collect details */
    map<string, string> info = this->getMachineInfo( uuid );
    
    /* Reset flags */
    session->flags = 0;
    
    /* Check state */
    if (info.find("State") != info.end()) {
        string state = info["State"];
        if (state.find("running") != string::npos) {
            session->state = STATE_STARTED;
        } else if (state.find("paused") != string::npos) {
            session->state = STATE_PAUSED;
        } else {
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
        int ans = this->exec("showhdinfo "+kk, &lines);
        if (ans == 0) {
        
            /* Tokenize data */
            diskinfo = tokenize( &lines, ':' );
            if (diskinfo.find("Logical size") != info.end()) {
                kk = diskinfo["Logical size"];
                kk = kk.substr(0, kk.length()-7); // Strip " MBytes"
                session->disk = ston<int>(kk);
            }
            
        }
    }
    
    /* Parse daemon information */
    string strProp;
    strProp = this->getProperty( uuid, "/CVMWeb/daemon/controlled" );
    if (strProp.empty()) {
        session->daemonControlled = false;
    } else {
        session->daemonControlled = (strProp.compare("1") == 0);
    }
    strProp = this->getProperty( uuid, "/CVMWeb/daemon/cap/min" );
    if (strProp.empty()) {
        session->daemonMinCap = 0;
    } else {
        session->daemonMinCap = ston<int>(strProp);
    }
    strProp = this->getProperty( uuid, "/CVMWeb/daemon/cap/max" );
    if (strProp.empty()) {
        session->daemonMaxCap = 100;
    } else {
        session->daemonMaxCap = ston<int>(strProp);
    }
    strProp = this->getProperty( uuid, "/CVMWeb/daemon/flags" );
    if (strProp.empty()) {
        session->daemonFlags = 0;
    } else {
        session->daemonFlags = ston<int>(strProp);
    }
    strProp = this->getProperty( uuid, "/CVMWeb/userData" );
    if (strProp.empty()) {
        session->userData = "";
    } else {
        session->userData = base64_decode(strProp);
    }
    
    /* Updated successfuly */
    return HVE_OK;
}

/**
 * Load session state from VirtualBox
 */
int Virtualbox::loadSessions() {
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    
    /* List the running VMs in the system */
    int ans = this->exec("list vms", &lines);
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
            updateSession( session );

            /* Register this session */
            CVMWA_LOG( "Info", "Registering session name=" << session->name << ", key=" << session->key << ", uuid=" << session->uuid << ", state=" << session->state  );
            this->registerSession(session);

        }
    }

    return 0;
}