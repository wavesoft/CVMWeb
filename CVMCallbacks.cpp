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

#include "CVMCallbacks.h"
#include <variant.h>

/**
 * Utility function to convert a VariantArgList vector to FB::VariantList vector
 */
FB::VariantList ArgVar2FBVar( VariantArgList& argVariants ) {
	FB::VariantList ans;
	for (std::vector< VariantArg >::iterator it = argVariants.begin(); it != argVariants.end(); ++it) {
		if ( int* pi = boost::get<int>( &(*it) ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( double* pi = boost::get<double>( &(*it) ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( float* pi = boost::get<float>( &(*it) ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( std::string* pi = boost::get<std::string>( &(*it) ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
	}
	return ans;
}

/**
 * Unregister everything upon destruction
 */
JSObjectCallbacks::~JSObjectCallbacks() {

	// Unregister from all the objects that we are monitoring
	for (std::vector< DisposableDelegate* >::iterator it = listening.begin(); it != listening.end(); ++it ) {
		DisposableDelegate * dd = *it;
		// Free memory and unregister
		delete dd;
	}

}

/**
 * Listen the events of the specified callback object
 */
void JSObjectCallbacks::listen( Callbacks & cb ) {

	// Register an anyEvent receiver and keep the slot reference
	listening.push_back(
		new DisposableDelegate( &cb, boost::bind( &JSObjectCallbacks::fire, this, _1, _2 ) )
	);
		
}

/**
 * Stop listening for events of the specified callback object
 */
void JSObjectCallbacks::stopListening( Callbacks & cb ) {

	// Register an anyEvent receiver and keep the slot reference
	for (std::vector< DisposableDelegate* >::iterator it = listening.begin(); it != listening.end(); ++it ) {
		DisposableDelegate * dd = *it;

		// Erase the callback item
		if (dd->cb == &cb) {

			// Erase item from vector
			listening.erase( it );

			// Delete and unregister
			delete dd;

			// Break from loop
			break;

		}

	}
		
}
/**
 * Fire an event to the javascript object
 */
void JSObjectCallbacks::fire( const std::string& name, VariantArgList& args ) {

	// First fire the anyEvent listeners
	for (std::vector< FB::JSObjectPtr >::iterator it = jsAny.begin(); it != jsAny.end(); ++it) {
		FB::JSObjectPtr jsobject = *it;

		// Convert the event name to onX...
		std::string cbName = "on" + toupper(name[0]) + name.substr(1);

		// Check if we have such callback
	    if (jsobject->HasProperty( cbName )) {

	    	// Fire callback
			jsobject->InvokeAsync( cbName, ArgVar2FBVar( args ) );

	    }

	}

	// Fetch named callbacks
	std::map< std::string, std::vector< FB::JSObjectPtr > >::iterator pos = jsNamed.find( name );
	if (pos == jsNamed.end()) return;

	// Fire callbacks
	std::vector< FB::JSObjectPtr > callbacks = (*pos).second;
	for (std::vector< FB::JSObjectPtr >::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
		FB::JSObjectPtr jsobject = *it;

    	// Fire anonymous callback
		jsobject->InvokeAsync( "", ArgVar2FBVar( args ) );

	}

}