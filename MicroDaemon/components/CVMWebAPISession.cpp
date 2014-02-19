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
#include "../daemon.h"

/**
 * Handle session commands
 */
void CVMWebAPISession::handleAction( const std::string& id, const std::string& action, ParameterMapPtr parameters ) {
	if (action == "start") {

		ParameterMapPtr startParm = parameters->subgroup("parameters");
		hvSession->start( startParm );

	} else if (action == "stop") {
		hvSession->stop();
	} else if (action == "pause") {
		hvSession->pause();
	} else if (action == "resume") {
		hvSession->resume();
	} else if (action == "hibernate") {
		hvSession->hibernate();
	} else if (action == "reset") {
		hvSession->reset();
	} else if (action == "close") {
		hvSession->close();
	}
}

/**
 * Handle timed event
 */
void CVMWebAPISession::processPeriodicJobs() {

	// Synchronize session state with VirtualBox (or file)
	CVMWA_LOG("Debug", "Syncing session");
	hvSession->update(false);

	// Check for API calls
    int sessionState = hvSession->local->getNum<int>("state", 0);
	CVMWA_LOG("Debug", "Session state: " << sessionState);
    if (sessionState == SS_RUNNING) {
    	if (!apiPortOnline) {

    		// Check if API port has gone online
    		bool newState = hvSession->isAPIAlive();
			CVMWA_LOG("Debug", "API Port: " << newState);
    		if (newState) {
	    		connection.sendEvent( "apiPortStateChanged", ArgumentList(true) );
    			apiPortOnline = true;
    		}

    	}
    } else {
    	if (apiPortOnline) {
    		// In any other state, the port is just offline
    		connection.sendEvent( "apiPortStateChanged", ArgumentList(false) );
    		apiPortOnline = false;
    	}
    }

}

/**
 * Handle state changed events and forward them if needed to the UI
 */
void CVMWebAPISession::__cbStateChanged( VariantArgList& args ) {
	connection.sendEvent( "stateChanged", args, uuid_str );

	// Check if we switched to a state where API is not available any more
	int session = boost::get<int>(args[0]);
	if ((session != SS_RUNNING) && apiPortOnline) {
		// In any other state, the port is just offline
		connection.sendEvent( "apiPortStateChanged", ArgumentList(false) );
		apiPortOnline = false;
	}

}

