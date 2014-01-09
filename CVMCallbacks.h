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

#include <vector>
#include <map>

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
	JSObjectCallbacks 						( ) : listening(), jsAny(), jsNamed() { };

	// Unregister everything upon destruction
	~JSObjectCallbacks						( );

	// Receive events from the specified callback object
	void listen								( Callbacks & ch );

	// Remove listener object
	void stopListening						( Callbacks & ch );

	// Register a named event listener
	void on 								( const std::string& name, const FB::variant &cb );

	// Unregister a named event listener
	void off 								( const FB::variant &cb );

	// Register an anyEvent listener
	void onAnyEvent							( const FB::variant &cb );

	// Remove an anyEvent listener
	void offAnyEvent						( const FB::variant &cb );

	// Trigger a custom event
	void fire								( const std::string& name, VariantArgList& args );

private:

	// The registry of the objects we are listening events for
	std::vector< DisposableDelegate* >							listening;

	// The JSObject pointer for anyEvent listeners
	std::vector< FB::JSObjectPtr >								jsAny;

	// The JSObject pointer for named event listeners
	std::map< std::string, std::vector< FB::JSObjectPtr > >		jsNamed;

};

/**
 * A helper class that binds the listener to the JSO
 */
class JSContextCallbackListener {
public:

	JSContextCallbackListener( JSObjectCallbacks& host, Callbacks & ch ): host(host), ch(ch) {
		host.listen( ch );
	};

	~JSContextCallbackListener( ) {
		host.stopListening( ch );
	};

private:

	JSObjectCallbacks& 			host;
	Callbacks& 					ch;

};

/**
 * Utility function to convert a VariantArgList vector to FB::VariantList vector
 */
FB::VariantList 		ArgVar2FBVar ( VariantArgList& argVariants );

#endif /* end of include guard: H_CVMWEBCALLBACKS */
