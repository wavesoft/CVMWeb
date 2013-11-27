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

// The VirtualBox PUEL License
#define VBOX_PUEL_LICENSE \
	"<strong>VirtualBox Personal Use and Evaluation License (PUEL)</strong>\n\n" \
	"License version 9, December 13, 2010\n\n" \
	"ORACLE CORPORATION (“ORACLE”) IS WILLING TO LICENSE THE EXTENSION PACK (AS DEFINED IN § 1 BELOW) TO YOU ONLY UPON THE CONDITION THAT YOU ACCEPT ALL OF THE TERMS CONTAINED IN THIS VIRTUALBOX PERSONAL USE AND EVALUATION LICENSE AGREEMENT (“AGREEMENT”). PLEASE READ THE AGREEMENT CAREFULLY. BY DOWNLOADING OR INSTALLING THE EXTENSION PACK, YOU ACCEPT THE FULL TERMS OF THIS AGREEMENT.\n\n" \
	"IF YOU ARE AGREEING TO THIS LICENSE ON BEHALF OF AN ENTITY OTHER THAN AN INDIVIDUAL PERSON, YOU REPRESENT THAT YOU ARE BINDING AND HAVE THE RIGHT TO BIND THE ENTITY TO THE TERMS AND CONDITIONS OF THIS AGREEMENT.\n\n" \
	"<strong>§ 1 Subject of Agreement.</strong> “Extension Pack”, as referred to in this Agreement, shall be the binary software package “Oracle VM VirtualBox Extension Pack”, an extension of the open-source VirtualBox virtualization software. VirtualBox allows for creating multiple virtual computers, each with different operating systems (“Guest Computers”), on a physical computer with a specific operating system (“Host Computer”) and for installing and executing these Guest Computers simultaneously. The Extension Pack consists of executable files in machine code for the Solaris, Windows, Linux, and Mac OS X operating systems as well as other data files as required by the executable files at run-time and documentation in electronic form. The Extension Pack includes all documentation and updates provided to You by Oracle under this Agreement, and the terms of this Agreement will apply to all such documentation and updates unless a different license is provided with an update or documentation.\n\n" \
	"<strong>§ 2 Grant of license.</strong> Oracle grants you a personal, non-exclusive, non-transferable, limited license without fees to reproduce, install, execute, and use internally the Extension Pack on a Host Computer for your Personal Use, Educational Use, or Evaluation. “Personal Use” requires that you use the Extension Pack on the same Host Computer where you installed it yourself and that no more than one client connect to that Host Computer at a time for the purpose of displaying Guest Computers remotely. “Educational use” is any use in an academic institution (schools, colleges and universities, by teachers and students). “Evaluation” means testing the Extension Pack for a reasonable period (that is, normally for a few weeks); after expiry of that term, you are no longer permitted to evaluate the Extension Pack.\n\n" \
	"<strong>§ 3 Restrictions and Reservation of Rights.</strong> (1) Any use beyond the provisions of § 2 is prohibited. The Extension Pack and copies thereof provided to you under this Agreement are copyrighted and licensed, not sold, to you by Oracle. Oracle reserves all copyrights and other intellectual property rights. This includes, but is not limited to, the right to modify, make available or public, rent out, lease, lend or otherwise distribute the Extension Pack. This does not apply as far as applicable law may require otherwise or if Oracle grants you additional rights of use in a separate agreement in writing.\n\n" \
	"(2) You may not do any of the following: (a) modify the Extension Pack. However if the documentation accompanying the Extension Pack lists specific portions of the Extension Pack, such as header files, class libraries, reference source code, and/or redistributable files, that may be handled differently, you may do so only as provided in the documentation; (b) rent, lease, lend or encumber the Extension Pack; (c) remove or alter any proprietary legends or notices contained in the Extension Pack; or (d) decompile, or reverse engineer the Extension Pack (unless enforcement of this restrictions is prohibited by applicable law).\n\n" \
	"(3) The Extension Pack is not designed, licensed or intended for use in the design, construction, operation or maintenance of any nuclear facility, and Oracle and its licensors disclaim any express or implied warranty of fitness for such uses.\n\n" \
	"(4) No right, title or interest in or to any trademark, service mark, logo or trade name of Oracle or its licensors is granted under this Agreement.\n\n" \
	"<strong>§ 4 Termination.</strong> The Agreement is effective on the Date you receive the Extension Pack and remains effective until terminated. Your rights under this Agreement will terminate immediately without notice from Oracle if you materially breach it or take any action in derogation of Oracle's and/or its licensors' rights to the Extension Pack. Oracle may terminate this Agreement should any product become, or in Oracle's reasonable opinion likely to become, the subject of a claim of intellectual property infringement or trade secret misappropriation. Upon termination, you will cease use of, and destroy, the Extension Pack and confirm compliance in writing to Oracle. Sections 3-9, inclusive, will survive termination of the Agreement.\n\n" \
	"<strong>§ 5 Disclaimer of Warranty.</strong> TO THE EXTENT NOT PROHIBITED BY APPLICABLE LAW, ORACLE PROVIDES THE EXTENSION PACK “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, EXCEPT TO THE EXTENT THAT THESE DISCLAIMERS ARE HELD TO BE LEGALLY INVALID. The entire risk as to the quality and performance of the Extension Pack is with you. Should it prove defective, you assume the cost of all necessary servicing, repair, or correction. In addition, Oracle shall be allowed to provide updates to the Extension Pack in urgent cases. You are then obliged to install such updates. Such an urgent case includes, but is not limited to, a claim of rights to the Extension Pack by a third party.\n\n" \
	"<strong>§ 6 Limitation of Liability.</strong> TO THE EXTENT NOT PROHIBITED BY APPLICABLE LAW, IN NO EVENT WILL ORACLE OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR SPECIAL, INDIRECT, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF OR RELATED TO THE USE OF OR INABILITY TO USE EXTENSION PACK, EVEN IF ORACLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. In no event will Oracle's liability to you, whether in contract, tort (including negligence), or otherwise, exceed the amount paid by you for the Extension Pack under this Agreement. Some states do not allow the exclusion of incidental or consequential damages, so some of the terms above may not be applicable to you.\n\n" \
	"<strong>§ 7 Third Party Code.</strong> Portions of the Extension Pack may be provided with notices and open source licenses from communities and third parties that govern the use of those portions, and any licenses granted hereunder do not alter any rights and obligations You may have under such open source licenses, however, the disclaimer of warranty and limitation of liability provisions in this Agreement will apply to all the Extension Pack.\n\n" \
	"<strong>§ 8 Export Regulations.</strong> The Extension Pack and all documents, technical data, and any other materials delivered under this Agreement are subject to U.S. export control laws and may be subject to export or import regulations in other countries. You agree to comply strictly with these laws and regulations and acknowledge that you have the responsibility to obtain any licenses to export, re-export, or import as may be required after delivery to you.\n\n" \
	"<strong>§ 9 U.S. Government Restricted Rights.</strong> If the Extension Pack is being acquired by or on behalf of the U.S. Government or by a U.S. Government prime contractor or subcontractor (at any tier), then the Government's rights in the Extension Pack and accompanying documentation will be only as set forth in this Agreement; this is in accordance with 48 CFR 227.7201 through 227.7202-4 (for Department of Defense (DOD) acquisitions) and with 48 CFR 2.101 and 12.212 (for non-DOD acquisitions).\n\n" \
	"<strong>§ 10 Miscellaneous.</strong> This Agreement is the entire agreement between you and Oracle relating to its subject matter. It supersedes all prior or contemporaneous oral or written communications, proposals, representations and warranties and prevails over any conflicting or additional terms of any quote, order, acknowledgment, or other communication between the parties relating to its subject matter during the term of this Agreement. No modification of this Agreement will be binding, unless in writing and signed by an authorized representative of each party. If any provision of this Agreement is held to be unenforceable, this Agreement will remain in effect with the provision omitted, unless omission would frustrate the intent of the parties, in which case this Agreement will immediately terminate. Course of dealing and other standard business conditions of the parties or the industry shall not apply. This Agreement is governed by the substantive and procedural laws of California and you and Oracle agree to submit to the exclusive jurisdiction of, and venue in, the courts in San Francisco, San Mateo, or Santa Clara counties in California in any dispute arising out of or relating to this Agreement.\n"

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
int 			vboxInstall( const DownloadProviderPtr & downloadProvider, const UserInteractionPtr & ui = UserInteractionPtr(), const FiniteTaskPtr & pf = FiniteTaskPtr(), int retries = 3 );



#endif /* end of include guard: VBOXCOMMON_H */
