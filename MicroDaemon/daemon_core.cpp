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

// Everything is included in daemon.h
// (Including cross-referencing)
#include "daemon.h"

#include <cstdlib>
#include <openssl/rand.h>
#include <Common/Utilities.h>

/**
 * Initialize daemon code
 */
DaemonCore::DaemonCore(): authKeys(), sessions(), keystore(), config() {

	// Initialize local config
    config = LocalConfig::global();

	// Detect and instantiate hypervisor
	hypervisor = detectHypervisor();
    if (hypervisor) {

        // Load stored sessions
        hypervisor->loadSessions();

    }

    // Initialize download provider
    downloadProvider = DownloadProvider::Default();

	// The daemon is running
	running = true;

}

/**
 * Check if daemon has exited
 */
bool DaemonCore::hasExited() {
	return !running;
}


/**
 * Check if a hypervisor was detected
 */
bool DaemonCore::hasHypervisor() {
    CRASH_REPORT_BEGIN;
    return (hypervisor->getType() != HV_NONE);
    CRASH_REPORT_END;
};

/**
 * Return the hypervisor name
 */
std::string DaemonCore::get_hv_name() {
    CRASH_REPORT_BEGIN;
    if (!hypervisor) {
        return "";
    } else {
        if (hypervisor->getType() == HV_VIRTUALBOX) {
            return "virtualbox";
        } else {
            return "unknown";
        }
    }
    CRASH_REPORT_END;
}

/**
 * Return hypervisor version
 */
std::string DaemonCore::get_hv_version() {
    CRASH_REPORT_BEGIN;
    if (!hypervisor) {
        return "";
    } else {
        return hypervisor->version.verString;
    }
    CRASH_REPORT_END;
}

/**
 * Allocate new authenticatino key
 */
std::string DaemonCore::newAuthKey() {
	AuthKey key;

	// The key lasts 5 minutes
	key.expireTime = getTimeInMs() + 300000;
	// Allocate new UUID
	key.key = newGUID();
	// Store on list
	authKeys.push_back( key );

	// Return the key
	return key.key;

}

/**
 * Validate authentication key
 */
bool DaemonCore::authKeyValid( const std::string& key ) {

	// Expire past keys
	bool found = false;
	unsigned long ts = getTimeInMs();
	for (std::list< AuthKey >::iterator it = authKeys.begin(); it != authKeys.end(); ++it) {
		AuthKey k = *it;
		if (ts >= k.expireTime) {
			authKeys.erase(it);
			if (it != authKeys.end())
				++it;
		} else if (k.key == key) {
			found = true;
		}
	}

	// Check if we found it
	return found;

}

/**
 * Calculate the domain ID using user's unique ID plus the domain name
 * specified.
 */
std::string DaemonCore::calculateHostID( std::string& domain ) {
    CRASH_REPORT_BEGIN;
    
    /* Fetch/Generate user UUID */
    std::string machineID = config->get("local-id");
    if (machineID.empty()) {
        machineID = keystore.generateSalt();
        config->set("local-id", machineID);
    }

    /* When we use the local-id, update the crash-reporting utility config */
    crashReportAddInfo("Machine UUID", machineID);
    
    /* Create a checksum of the user ID + domain and use this as HostID */
    std::string checksum = "";
    sha256_buffer( machineID + "|" + domain, &checksum );
    return checksum;

    CRASH_REPORT_END;
}

/**
 * Store the given session and return it's unique ID
 */
CVMWebAPISession* DaemonCore::storeSession( DaemonConnection& connection, HVSessionPtr hvSession ) {

    // Create a random int that does not exist in the sessions
    int uuid;
    while (true) {
        
        // Create a positive random number
        RAND_bytes( (unsigned char*)&uuid, 4 );
        uuid = abs(uuid);

        // Make sure we have no collisions
        if (sessions.find(uuid) == sessions.end())
            break;
            
    }

    // Create CVMWebAPISession wrapper and store it on sessionss
    CVMWebAPISession* cvmSession = new CVMWebAPISession( *this, connection, hvSession, uuid );
    sessions[uuid] = cvmSession;

    // Return session
    return cvmSession;

}

/**
 * Unregister all sessions launched from the given connection
 */
void DaemonCore::releaseConnectionSessions( DaemonConnection& connection ) {
    CVMWA_LOG("Debug", "Releasing connection sessions");
    for (std::map<int, CVMWebAPISession* >::iterator it = sessions.begin(); it != sessions.end(); ++it) {
        CVMWebAPISession* sess = (*it).second;
        if (&sess->connection == &connection) {

            // Release wrapper object
            delete sess;

            // Remove from list
            sessions.erase( it );
            if (sessions.empty()) break;
            if (it != sessions.end()) it++;

        }
    }

}

/**
 * Forward the tick event to all of the child nodes
 */
void DaemonCore::processPeriodicJobs() {
    for (std::map<int, CVMWebAPISession* >::iterator it = sessions.begin(); it != sessions.end(); ++it) {
        CVMWebAPISession* sess = (*it).second;
        sess->processPeriodicJobs();
    }   
}

