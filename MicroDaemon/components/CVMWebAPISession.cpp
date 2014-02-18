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

#include "CVMWebAPISession.h"

/**
 * Handle session commands
 */
void CVMWebAPISession::handleAction( const std::string& id, const std::string& action, ParameterMapPtr parameters ) {
	if (action == "start") {
		hvSession->start(NULL);
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
	}
}

/**
 * Forward state changed events to the UI
 */
void CVMWebAPISession::__cbStateChanged( VariantArgList& args ) {
	session.sendEvent("stateChanged", "", args);
}