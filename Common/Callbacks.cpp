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

#include "Callbacks.h"


/**
 * Register a callback that handles a named event
 */
void CallbackHost::on ( const std::string& name, const cbNamedEvent& cb ) {
	std::std::vector< cbNamedEvent > cbs;
	if (namedEventCallbacks.find(name) != namedEventCallbacks.end())
		cbs = namedEventCallbacks[name];
	cbs.push_back(cb);
	namedEventCallbacks[name] = cbs;
}

/**
 * Register a callback that handles all the events
 */
void CallbackHost::onAnyEvent ( const cbAnyEvent & cb ) {
	anyEventCallbacks.push_back(cb);
}

/**
 * Fire an event by it's name
 */
void CallbackHost::fire( const std::string& name, VariantArgList& args ){

	// First, call the anyEvent handlers
	for (std::vector< cbAnyEvent >::iterator it = anyEventCallbacks.begin(); it != anyEventCallbacks.end(); ++it) {
		cbAnyEvent cb = *it;
		try {
			if (cb) cb( name, args );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while forwarding event to cbAnyEvent")
		}
	}

	// Then, call the named event hanlers
	std::std::vector< cbNamedEvent > cbs;
	if (namedEventCallbacks.find(name) != namedEventCallbacks.end())
		cbs = namedEventCallbacks[name];
	
	for (std::vector< cbNamedEvent >::iterator it = cbs.begin(); it != cbs.end(); ++it) {
		cbNamedEvent cb = *it;
		try {
			if (cb) cb( args );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while forwarding event to cbNamedEvent")
		}
	}

}
