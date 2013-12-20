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
#ifndef H_CVMWEBCALLBACKS
#define H_CVMWEBCALLBACKS

#include "Common/Callbacks.h"

#include "JSObject.h"
#include "variant_list.h"

/**
 * A utility class that keeps track of the delegated Callbacks
 */
class _DelegatedSlot {
public:

	/**
	 * Connect to the anyEvent slot at constructor
	 */	
	_DelegatedSlot( const Callbacks & cb, cbAnyEvent callback ) : cb(cb), slot() {
		slot = cb.onAnyEvent( callback );
	}

	/**
	 * Disconnect from the event slot on destruction
	 */	
	~_DelegatedSlot() {
		cb.offAnyEvent( slot );
	}

private:
	const Callbacks & 	cb;
	AnyEventSlotPtr 	slot;

};

/**
 * Wrapper class that forawrds events to a javascript object callback
 *
 * The javascript object automatically binds to named Callback events, using
 * the 'on-' callback. For example:
 *
 * {
 *     onBegin: function( msg ) {
 *			...
 *     },
 *     onProgress: function( msg, v ) {
 *			...
 *     }
 * }
 *
 */
class JSObjectCallbacks {
public:

	// Create an object that can forward callback events to a given javascript object
	JSObjectCallbacks 						( const FB::variant &cb );

	// Receive events from the specified callback object
	void listen								( const Callbacks & ch );

	// Trigger a custom event
	void fire								( const std::string& name, VariantArgList args );

private:

	// The registry of the objects we are listening events for
	std::vector< _DelegatedSlot >			delegateSlots;

	// The JSObject pointer for the javascript object we wrap
	FB::JSObjectPtr 						jsobject;

	// This flag is set to TRUE if the jsobject is valid
	bool									isAvailable;

};

/**
 * Utility function to convert a VariantArgList vector to FB::VariantList vector
 */
FB::VariantList 		ArgVar2FBVar ( VariantArgList& argVariants );

#endif /* end of include guard: H_CVMWEBCALLBACKS */
