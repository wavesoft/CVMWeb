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

#include "CVMWebInteraction.h"

/**
 * Create an instance of a UserInterface interaction that forwards the events
 * to a javascript callback object.
 */
CVMWebInteraction::CVMWebInteraction( const FB::variant &cb ) : cb(cb) {

	// Work only if we have a valid object specified
	if (!IS_MISSING(jsobject) && cb.is_of_type<FB::JSObjectPtr>()) {

		// Extract javascript object
		jsobject = cb.cast<FB::JSObjectPtr>();

		// Mark as available
		isAvailable = true;

		// Bind callbacks
		setConfirmHandler( boost::bind( &CVMWebInteraction::__callbackConfim, this, _1, _2, _3 ) );
		setAlertHandler( boost::bind( &CVMWebInteraction::__callbackAlert, this, _1, _2, _3 ) );
		setLicenseHandler( boost::bind( &CVMWebInteraction::__callbackLicense, this, _1, _2, _3 ) );
		setLicenseURLHandler( boost::bind( &CVMWebInteraction::__callbackLicenseURL, this, _1, _2, _3 ) );

	}
}

/**
 * Static function to create a shared pointer
 */
CVMWebInteractionPtr CVMWebInteraction::fromJSObject( const FB::variant &cb ) {
	return boost::make_shared< CVMWebInteraction >( cb );
}

/**
 * Callback function to ask javascript for confirmation
 */
void CVMWebInteraction::__callbackConfim (const std::string& title, const std::string& message, const callbackResult& cb ) {

	// If we don't have a way to notify the user, just abort it
	if (!isAvailable) {
		cb( UI_UNDEFINED );
		return;
	}

	// Check if we have such callback
    if (jsobject->HasProperty( "confirm" )) {
    	// Fire callback
		jsobject->InvokeAsync( "confirm", FB::variant_list_of( 
			boost::make_shared<CVMWebInteractionMessage>( title, message, cb, "confirm" )
		));
    }
}

/**
 * Callback function to ask javascript for alert
 */
void CVMWebInteraction::__callbackAlert (const std::string&, const std::string&, const callbackResult& cb) {

	// If we don't have a way to notify the user, just abort it
	if (!isAvailable) {
		cb( UI_UNDEFINED );
		return;
	}

	// Check if we have such callback
    if (jsobject->HasProperty( "alert" )) {
    	// Fire callback
		jsobject->InvokeAsync( "alert", FB::variant_list_of( 
			boost::make_shared<CVMWebInteractionMessage>( title, message, cb, "alert" )
		));
    }
}

/**
 * Callback function to ask javascript for licence acceptance
 */
void CVMWebInteraction::__callbackLicense (const std::string&, const std::string&, const callbackResult& cb) {

	// If we don't have a way to notify the user, just abort it
	if (!isAvailable) {
		cb( UI_UNDEFINED );
		return;
	}

	// Check if we have such callback
    if (jsobject->HasProperty( "confirmLicenseURL" )) {
    	// Fire callback
		jsobject->InvokeAsync( "confirmLicense", FB::variant_list_of( 
			boost::make_shared<CVMWebInteractionMessage>( title, message, cb, "confirmLicense" )
		));
    }
}

/**
 * Callback function to ask javascript for license acceptance by it's URL
 */
void CVMWebInteraction::__callbackLicenseURL (const std::string&, const std::string&, const callbackResult& cb) {

	// If we don't have a way to notify the user, just abort it
	if (!isAvailable) {
		cb( UI_UNDEFINED );
		return;
	}

	// Check if we have such callback
    if (jsobject->HasProperty( "confirmLicenseURL" )) {
    	// Fire callback
		jsobject->InvokeAsync( "confirmLicenseURL", FB::variant_list_of( 
			boost::make_shared<CVMWebInteractionMessage>( title, message, cb, "confirmLicenseURL", "title", "url" )
		));
    }
}
