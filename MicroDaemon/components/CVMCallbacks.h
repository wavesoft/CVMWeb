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
#ifndef DAEMON_COMPONENT_CALLBACKS_H
#define DAEMON_COMPONENT_CALLBACKS_H

#include <Common/Callbacks.h>
#include "../web/api.h"

/**
 * A utility class that keeps track of the delegated Callbacks
 * and helps deregistering the listeners after destruction.
 */
class DisposableDelegate {
public:

	/**
	 * Connect to the anyEvent slot at constructor
	 */	
	DisposableDelegate( Callbacks* cb, cbAnyEvent callback ) : cb(cb), slot() {
		slot = cb->onAnyEvent( callback );
	}

	/**
	 * Disconnect from the event slot on destruction
	 */	
	~DisposableDelegate() {
		cb->offAnyEvent( slot );
	}

	/**
	 * The callback object used for unregister
	 */
	Callbacks*			cb;

	/**
	 * The allocated slot used for unregister
	 */
	AnyEventSlotPtr 	slot;

};

/**
 * Wrapper class that forawrds events to the remote end of the socket.
 */
class CVMCallbacks {
public:

	// Constructor
	CVMCallbacks( WebsocketAPI& api, const std::string& eventID ) 
		: listening(), api(api), eventID(eventID) { };

	// Destructor
	~CVMCallbacks();

	// Trigger a custom event
	void fire								( const std::string& name, VariantArgList& args );

	// Receive events from the specified callback object
	void listen								( Callbacks & ch );

	// Remove listener object
	void stopListening						( Callbacks & ch );

private:

	// The registry of the objects we are listening events for
	std::vector< DisposableDelegate* >		listening;

	// The API session to send messages to
	WebsocketAPI&							api;

	// The current event ID
	const std::string&						eventID;

};

#endif /* end of include guard: DAEMON_COMPONENT_CALLBACKS_H */
