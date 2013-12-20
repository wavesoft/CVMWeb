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
		if ( int* pi = boost::get<int>( it ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( double* pi = boost::get<double>( it ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( float* pi = boost::get<float>( it ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
		else if ( std::string* pstr = boost::get<std::string>( &operand ) )
			ans.insert( ans.end(), FB::variant( *pi ) );
	}
	return ans;
}

/**
 * Prepare the Javascipt Object Callback
 */
JSObjectCallbacks::JSObjectCallbacks( const Callbacks & ch, const FB::variant &cb ) : parent(ch), isAvailable(false) {
	if (!IS_MISSING(jsobject) && cb.is_of_type<FB::JSObjectPtr>()) {

		// Extract javascript object
		jsobject = cb.cast<FB::JSObjectPtr>();

		// Mark as available
		isAvailable = true;

		// Bind to parent
		delegateCallback = boost::bind( &JSObjectCallbacks::_delegate_anyEvent, this, _1, _2 );
		ch.onAnyEvent( delegateCallback );

	}
}

/**
 * Unregister from the parent callback when destroyed
 */
JSObjectCallbacks::~JSObjectCallbacks( ) {

	// Unregister this class from the callbacks
	if (isAvailable)
		ch.offAnyEvent( delegateCallback );
	
}

/**
 * Delegate function to forward any named event to a javascript object
 */
void JSObjectCallbacks::_delegate_anyEvent( const std::string& name, VariantArgList& args ) {
	if (!isAvailable) return;

	// Convert the event name to onX...
	std::string cbName = "on" + toupper(name[0]) + name.substr(1);

	// Check if we have such callback
    if (jsobject->HasProperty( cbName )) {

    	// Fire callback
		jsobject->InvokeAsync( cbName, ArgVar2FBVar( args ) );

    }

}
