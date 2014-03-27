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
NamedEventSlotPtr Callbacks::on ( const std::string& name, cbNamedEvent cb ) {
	boost::mutex::scoped_lock lock(shopMutex);

    // Allocate missing entry
	if (namedEventCallbacks.find(name) == namedEventCallbacks.end())
        namedEventCallbacks[name] = std::vector< NamedEventSlotPtr >();
    // Update map
    std::vector< NamedEventSlotPtr > * cbs = &namedEventCallbacks[name];
    NamedEventSlotPtr ptr = boost::make_shared<NamedEventSlot>( cb );
	cbs->push_back( ptr );
	return ptr;
}

/**
 * Unegister a callback that handles a named event
 */
void Callbacks::off ( const std::string& name, NamedEventSlotPtr cb ) {
	if (!cb) return;
	boost::mutex::scoped_lock lock(shopMutex);

    // Allocate missing entry
	if (namedEventCallbacks.find(name) == namedEventCallbacks.end()) return;
    // Lookup and delete entry from map
    std::vector< NamedEventSlotPtr > * cbs = &namedEventCallbacks[name];
    for (std::vector< NamedEventSlotPtr >::iterator it = cbs->begin(); it != cbs->end(); ++it) {
        if (*it == cb) {
        	CVMWA_LOG("Callbacks", "Found and erased");
            cbs->erase(it);
            return;
        }
    }
}

/**
 * Register a callback that handles all the events
 */
AnyEventSlotPtr Callbacks::onAnyEvent ( cbAnyEvent cb ) {
	boost::mutex::scoped_lock lock(shopMutex);
	AnyEventSlotPtr ptr = boost::make_shared<AnyEventSlot>( cb );
	anyEventCallbacks.push_back( ptr );
	return ptr;
}

/**
 * Unregister a callback that handles all the events
 */
void Callbacks::offAnyEvent ( AnyEventSlotPtr cb ) {
	if (!cb) return;
	boost::mutex::scoped_lock lock(shopMutex);

	// Find and erase the given anyEvent slot
	for (std::vector< AnyEventSlotPtr >::iterator it = anyEventCallbacks.begin(); it != anyEventCallbacks.end(); ++it) {
	    if (*it == cb) {
		    anyEventCallbacks.erase(it);
            return;
	    }
    }

}

/**
 * Fire an event by it's name
 */
void Callbacks::fire( const std::string& name, VariantArgList& args ){
	boost::mutex::scoped_lock lock(shopMutex);

	// First, call the anyEvent handlers
	for (std::vector< AnyEventSlotPtr >::iterator it = anyEventCallbacks.begin(); it != anyEventCallbacks.end(); ++it) {
		AnyEventSlotPtr cb = *it;
		try {
			if (cb) cb->callback( name, args );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while forwarding event to cbAnyEvent")
		}
	}

	// Then, call the named event hanlers
	if (namedEventCallbacks.find(name) != namedEventCallbacks.end()) {
    	std::vector< NamedEventSlotPtr > * cbs = &namedEventCallbacks[name];
        for (std::vector< NamedEventSlotPtr >::iterator it = cbs->begin(); it != cbs->end(); ++it) {
	        NamedEventSlotPtr cb = *it;
	        try {
		        if (cb) cb->callback( args );
	        } catch (...) {
		        CVMWA_LOG("Error", "Exception while forwarding event to cbNamedEvent")
	        }
        }
    }

}
