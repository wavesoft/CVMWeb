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

#pragma once
#ifndef VBOXCOMMON_H
#define VBOXCOMMON_H

#include "Common/ProgressFeedback.h"
#include "Common/Hypervisor.h"
#include "VBoxStateStore.h"

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

// Create some condensed strings using the above parameters
#define BOOT_DSK            BOOT_CONTROLLER " (" BOOT_PORT ", " BOOT_DEVICE ")"
#define SCRATCH_DSK         SCRATCH_CONTROLLER " (" SCRATCH_PORT ", " SCRATCH_DEVICE ")"
#define CONTEXT_DSK         CONTEXT_CONTROLLER " (" CONTEXT_PORT ", " CONTEXT_DEVICE ")"
#define GUESTADD_DSK        GUESTADD_CONTROLLER " (" GUESTADD_PORT ", " GUESTADD_DEVICE ")"
#define FLOPPYIO_DSK        FLOPPYIO_CONTROLLER " (" FLOPPYIO_PORT ", " FLOPPYIO_DEVICE ")"

/////////////////////////////////
// Global forward declerations
/////////////////////////////////

// Static definition
class VBoxSession;
class VBoxInstance;

// Shared pointer definition
typedef boost::shared_ptr< VBoxSession >	VBoxSessionPtr;
typedef boost::shared_ptr< VBoxInstance >  	VBoxInstancePtr;

/////////////////////////
// Local tool functions
/////////////////////////

/**
 * Extract the mac address of the VM from the NIC line definition
 */
std::string _vbox_extractMac( std::string nicInfo );

/**
 * Replace the last part of the given IP
 */
std::string _vbox_changeUpperIP( std::string baseIP, int value );

/////////////////////////
// Exported functions
/////////////////////////

/* Global function to try to instantiate a VirtualBox Hypervisor */
HVInstancePtr 	vboxDetect();

/* Global function to try to install a VirtualBox Hypervisor */
bool 			vboxInstall( const FiniteTaskPtr & pf = FiniteTaskPtr() );



#endif /* end of include guard: VBOXCOMMON_H */
