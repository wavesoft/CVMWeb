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

#include "UserInteraction.h"
#include <boost/bind.hpp>

// Initialize singleton to default
UserInteraction::defaultSingleton;

/**
 * Return default user interaction pointer
 */
UserInteractionPtr UserInteraction::Default() {

	// Automatically create an 'accept' singleton
	// if it's not still there.
	if (!defaultSingleton)
		defaultSingleton = boost::make_shared<AcceptInteraction>();

	// Return singleton isntance
	return defaultSingleton;

}

/**
 * Display the specified message and wait for an OK/Cancel response.
 */
int UserInteraction::confirm ( const std::string & message, int timeout = 0 ) {
	if (!cbConfirm) return UI_UNDEFINED;
	cbConfirm( message, bind::boost( &UserInteraction::__cbCallback, this ) );
	return __waitResult( timeout );
}

/**
 * Display the specified message and wait until the user clicks OK.
 */
int UserInteraction::alert ( const std::string& message, int timeout = 0 ) {
	if (!cbAlert) return UI_UNDEFINED;
	cbAlert( message, bind::boost( &UserInteraction::__cbCallback, this ) );
	return __waitResult( timeout );
}

/**
 * Display a licence whose contents is fetched from the given URL and
 * wait for user response for accepting or declining it.
 */
int UserInteraction::confirmLicense	( const std::string& url, int timeout = 0 ) {
	if (!cbLicense) return UI_UNDEFINED;
	cbLicense( url, bind::boost( &UserInteraction::__cbCallback, this ) );
	return __waitResult( timeout );
}

/**
 * Define a handler for confirm message
 */
int UserInteraction::setConfirmHandler	( const callbackConfirm & cb ) {
	cbConfirm = cb;
}

/**
 * Define a handler for alert message
 */
int UserInteraction::setAlertHandler ( const callbackAlert & cb ) {
	cbAlert = cb;
}

/**
 * Define a handler for alert message
 */
int UserInteraction::setLicenseHandler ( const callbackLicense & cb ) {
	cbLicense = cb;
}

/** 
 * Local function to wait for callback
 */
int UserInteraction::__waitResult ( int timeout ) {

	// Set result to -1 (Pending response)
	result = -1;

	// Wait on mutex
	boost::unique_lock<boost::mutex> lock(mutex);
	while(result < 0) {
		cond.wait(lock);
	}

	// Return result
	return result;

}

/**
 * Callback function for receiving feedback from the callback handlers
 */
void UserInteraction::__cbCallback( int result ) {

	// If result is negative, switch to '0' = Undefined
	if (result < 0) result = 0;

	// Update result
    {
        boost::unique_lock<boost::mutex> lock(fsmtPauseMutex);
        this->result = result;
    }

    // Release mutex
    cond.notify_all();

}