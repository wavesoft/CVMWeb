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

#ifndef VBOXCOMMON_H
#define VBOXCOMMON_H

#include "Common/Hypervisor.h"
#include "VBoxStateStore.h"

/* Forward-declare the classes */
class VBoxSession;
class VBoxHypervisor;

/* Global function to try to instantiate a VirtualBox Hypervisor */
Hypervisor * 	vboxDetect();

/* Global function to try to install a VirtualBox Hypervisor */
bool 			vboxInstall();


#endif /* end of include guard: VBOXCOMMON_H */
