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
#ifndef DAEMON_CORE_H
#define DAEMON_CORE_H

#include <Common/Hypervisor.h>
#include <Common/DomainKeystore.h>
#include <Common/DownloadProvider.h>

class CVMWebAPISession;

#include "components/CVMWebAPISession.h"

class AuthKey {
public:

	/**
	 * The key string
	 */
	std::string			key;

	/**
	 * The time after the key is considered invalid
	 */
	unsigned long 		expireTime;

};

class DaemonCore {
public:

	/**
	 * Initialize daemon core
	 */
	DaemonCore();

	/**
	 * Check if the daemon has exited
	 */
	bool hasExited();

	/**
	 * Allocate new authenticatino key
	 */
	std::string 				newAuthKey();

	/**
	 * Validate authentication key
	 */
	bool 						authKeyValid( const std::string& key );

	/**
	 * Calculate the host ID for the given domain
	 */
	std::string 				calculateHostID( std::string& domain );

	/**
	 * Create new random session ID and store the given session
	 */
	int 						storeSession( CVMWebAPISession* session );

	/**
	 * Check if a hypervisor was detected
	 */
	bool 						hasHypervisor();

	/**
	 * Return the hypervisor name
	 */
	std::string 				get_hv_name();

	/**
	 * Return hypervisor version
	 */
	std::string 				get_hv_version();

public:

	/**
	 * The identified hypervisor
	 */
	HVInstancePtr								hypervisor;

	/**
	 * Connection status
	 */
	bool 										running;

	/**
	 * The domain keystore
	 */
	DomainKeystore								keystore;

	/**
	 * Local config
	 */
	LocalConfigPtr								config;

	/**
	 * The download provider
	 */
	DownloadProviderPtr							downloadProvider;

	/**
	 * Authenticated keys
	 */
	std::list< AuthKey >						authKeys;

	/**
	 * Sessions
	 */
	std::map<unsigned int, CVMWebAPISession* >	sessions;

};

#endif /* end of include guard: DAEMON_CORE_H */
