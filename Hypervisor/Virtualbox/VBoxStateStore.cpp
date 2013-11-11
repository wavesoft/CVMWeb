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

#include "VBoxStateStore.h"

/**
 * Return a LocalConfig instance that points to the specified state file
 */
LocalConfigPtr VBoxStateStore::configFor( const std::string& uuid ) {

	// Return a LocalConfig instance that points to the given
	// map file in the run directory.
	return boost::make_shared<LocalConfig>(
			getAppDataPath() + "/run", 	// Configuration is stored on <appdata>/run
			"vbox-state-" + uuid		// Filename is 'vbox-state-12345678901234567890.map'
		);

}
