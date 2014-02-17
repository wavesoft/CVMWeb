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

#include "daemon_session.h"
#include <vector>

#include <json/json.h>

#include <Common/Hypervisor.h>
#include <Common/ProgressFeedback.h>

#include "components/CVMCallbacks.h"

/**
 * Handle incoming websocket action
 */
void DaemonSession::handleAction( const std::string& id, const std::string& action, ParameterMapPtr parameters ) {
	std::map<std::string, std::string> data;

	if (action == "handshake") {

		// Reply the version information about this
		data["version"] = "1.0";
		reply(id, data);

		// Check if we are privileged
		if (parameters->contains("auth")) {
			privileged = core.authKeyValid( parameters->get("auth") );

            std::vector<std::string> arr;
            arr.push_back("1.0");
			sendEvent("log", "", arr );
		}

	} else if (action == "requestSession") {

		// Request session
		

	} else if ( action == "exit" ) {

		// Shut down core
		core.running = false;

	}

}

void DaemonSession::requestSession_thread( const std::string& id, const std::string& vmcpURL, ParameterMapPtr parameters ) {
	CRASH_REPORT_BEGIN;
	HVInstancePtr hv = core.hypervisor;

    // Create the object where we can forward the events
    CVMCallbacks cb( *this, id );

    try {

        // Create a progress feedback mechanism
        FiniteTaskPtr pTasks = boost::make_shared<FiniteTask>();
        pTasks->setMax( 2 );
        cb.listen( *pTasks.get() );

        // Create two sub-tasks that will be used for equally
        // dividing the progress into two tasks: validate and start
        FiniteTaskPtr pInit = pTasks->begin<FiniteTask>( "Preparing for session request" );
        pInit->setMax( 4 );

        // =======================================================================

        // Wait for delaied hypervisor initiation
        hv->waitTillReady( pInit->begin<FiniteTask>( "Initializing hypervisor" ) );

        // =======================================================================

        // Try to update authorized keystore if it's in an invalid state
        pInit->doing("Initializing crypto store");
        if (!privileged) {
        
            // Trigger update in the keystore (if it's nessecary)
            core.keystore.updateAuthorizedKeystore( core.downloadProvider );

            // Still invalid? Something's wrong
            if (!core.keystore.valid) {
                cb.fire("failed", ArgumentList( "Unable to initialize cryptographic store" )( HVE_NOT_VALIDATED ) );
                return;
            }

            // Block requests from untrusted domains
            if (!core.keystore.isDomainValid(domain)) {
                cb.fire("failed", ArgumentList( "The domain is not trusted" )( HVE_NOT_TRUSTED ) );
                return;
            }
        
        }
        pInit->done("Crypto store initialized");
    
        // =======================================================================

        // Validate arguments
        pInit->doing("Contacting the VMCP endpoint");
    
        // Put salt and user-specific ID in the URL
        std::string salt = core.keystore.generateSalt();
        std::string glueChar = "&";
        if (vmcpURL.find("?") == std::string::npos) glueChar = "?";
        std::string newURL = 
            vmcpURL + glueChar + 
            "cvm_salt=" + salt + "&" +
            "cvm_hostid=" + core.calculateHostID( domain );
    
        // Download data from URL
        std::string jsonString;
        int res = core.downloadProvider->downloadText( newURL, &jsonString );
        if (res < 0) {
            cb.fire("failed", ArgumentList( "Unable to contact the VMCP endpoint" )( res ) );
            return;
        }

        pInit->doing("Validating VMCP data");

        // Try to parse the data
        Json::Value jsonData;
        Json::Reader jsonReader;
        bool parsingSuccessful = jsonReader.parse( jsonString, jsonData );
        if ( !parsingSuccessful ) {
            // report to the user the failure and their locations in the document.
            cb.fire("failed", ArgumentList( "Unable to parse response data as JSON" )( HVE_QUERY_ERROR ) );
            return;
        }
    
        // Import response to a ParameterMap
        ParameterMapPtr vmcpData = ParameterMap::instance();
        vmcpData->fromJSON(jsonData);

        // Validate response
        if (!vmcpData->contains("name")) {
            cb.fire("failed", ArgumentList( "Missing 'name' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (!vmcpData->contains("secret")) {
            cb.fire("failed", ArgumentList( "Missing 'secret' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (!vmcpData->contains("signature")) {
            cb.fire("failed", ArgumentList( "Missing 'signature' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (vmcpData->contains("diskURL") && !vmcpData->contains("diskChecksum")) {
            cb.fire("failed", ArgumentList( "A 'diskURL' was specified, but no 'diskChecksum' was found in the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        }

        // Validate signature
        res = core.keystore.signatureValidate( domain, salt, vmcpData );
        if (res < 0) {
            cb.fire("failed", ArgumentList( "The VMCP response signature could not be validated" )( res ) );
            return;
        }

        pInit->done("Obtained information from VMCP endpoint");

        // =======================================================================
    
        // Check session state
        res = hv->sessionValidate( vmcpData );
        if (res == 2) { 
            // Invalid password
            cb.fire("failed", ArgumentList( "The password specified is invalid for this session" )( HVE_PASSWORD_DENIED ) );
            return;
        }

        // =======================================================================
    
        /* Check if the session is new and prompt the user */
        pInit->doing("Validating request");
        if (res == 0) {
            pInit->doing("Session is new, asking user for confirmation");

            // Newline-specific split
            std::string msg = "The website " + domain + " is trying to allocate a " + core.get_hv_name() + " Virtual Machine \"" + vmcpData->get("name") + "\". This website is validated and trusted by CernVM." _EOL _EOL "Do you want to continue?";

            // Prompt user using the currently active userInteraction 
            if (userInteraction->confirm("New CernVM WebAPI Session", msg) != UI_OK) {
            
                // If we were aborted due to shutdown, exit
                if (core.hasExited()) return;

                // Manage throttling 
                if ((getMillis() - this->throttleTimestamp) <= THROTTLE_TIMESPAN) {
                    if (++this->throttleDenies >= THROTTLE_TRIES)
                        this->throttleBlock = true;
                } else {
                    this->throttleDenies = 1;
                    this->throttleTimestamp = getMillis();
                }

                // Fire error
                cb.fire("failed", ArgumentList( "User denied the allocation of new session" )( HVE_ACCESS_DENIED ) );
                return;
            
            } else {
            
                // Reset throttle
                this->throttleDenies = 0;
                this->throttleTimestamp = 0;
            
            }
        
        }
        pInit->done("Request validated");

        // =======================================================================

        // Prepare a progress task that will be used by sessionOpen    
        FiniteTaskPtr pOpen = pTasks->begin<FiniteTask>( "Oppening session" );

        // Open/resume session
        HVSessionPtr session = hv->sessionOpen( vmcpData, pOpen );
        if (!session) {
            cb.fire("failed", ArgumentList( "Unable to open session" )( HVE_ACCESS_DENIED ) );
            return;
        }

        // We have everything. Prepare CVMWebAPI Session and fire success
        // >>>>>>> HERE >>>>>>>
        // boost::shared_ptr<CVMWebAPISession> pSession = boost::make_shared<CVMWebAPISession>(p, m_host, session );
        // <<<<<<<<<<<<<<<<<<<<
        pTasks->complete( "Session oppened successfully" );
        cb.fire("succeed", ArgumentList( "Session oppened successfully" ) );
        
        // Check if we need a daemon for our current services
        hv->checkDaemonNeed();
    
    } catch (...) {

        CVMWA_LOG("Error", "Exception occured!");

        // Raise failure
        cb.fire("failed", ArgumentList( "Unexpected exception occured while requesting session" )( HVE_EXTERNAL_ERROR ) );

    }

    CRASH_REPORT_END;
}