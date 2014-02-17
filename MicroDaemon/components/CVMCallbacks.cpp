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

/**
 * Utility function to convert a VariantArgList vector to Json::Value array
 */
Json::Value ArgVal2Json( VariantArgList& argVariants ) {
	Json::Value val;
	for (std::vector< VariantArg >::iterator it = argVariants.begin(); it != argVariants.end(); ++it) {
		if ( int* pi = boost::get<int>( &(*it) ) )
			val.append( *pi );
		else if ( double* pi = boost::get<double>( &(*it) ) )
			val.append( *pi );
		else if ( float* pi = boost::get<float>( &(*it) ) )
			val.append( *pi );
		else if ( std::string* pi = boost::get<std::string>( &(*it) ) )
			val.append( *pi );
	}
	return val;
}

/**
 * Unregister everything upon destruction
 */
CVMCallbacks::~CVMCallbacks() {

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
void CVMCallbacks::listen( Callbacks & cb ) {

	// Register an anyEvent receiver and keep the slot reference
	listening.push_back(
		new DisposableDelegate( &cb, boost::bind( &CVMCallbacks::fire, this, _1, _2 ) )
	);
		
}

/**
 * Stop listening for events of the specified callback object
 */
void CVMCallbacks::stopListening( Callbacks & cb ) {

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
void CVMCallbacks::fire( const std::string& name, VariantArgList& args ) {
	Json::Value value = ArgVal2Json( args );
	std::vector< std::string > ans;

	for (std::vector< VariantArg >::iterator it = args.begin(); it != args.end(); ++it) {
		if ( int* pi = boost::get<int>( &(*it) ) )
			ans.push_back( ntos<int>(*pi) );
		else if ( double* pi = boost::get<double>( &(*it) ) )
			ans.push_back( ntos<double>(*pi) );
		else if ( float* pi = boost::get<float>( &(*it) ) )
			ans.push_back( ntos<float>(*pi) );
		else if ( std::string* pi = boost::get<std::string>( &(*it) ) )
			ans.push_back( *pi );
	}

	// Forward the event to the interface
	api.sendEvent( name, eventID, ans );

}
