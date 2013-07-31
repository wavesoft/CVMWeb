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

#ifndef CONFIG_H
#define CONFIG_H

/**
 * The URL that contains the list of the downloads the plugin can perform.
 *
 * Currently this is the hypervisor for the different platforms and the VirtualBox
 * extension pack.
 *
 */
#define		URL_DOWNLOAD_SOURCES			"http://labs.wavesoft.gr/lhcah"

/**
 * The signature for the SRC_DOWNLOAD_SOURCES file
 */
#define		URL_DOWNLOAD_SOURCES_SIG		"http://labs.wavesoft.gr/lhcah/downloads.sig"


#endif