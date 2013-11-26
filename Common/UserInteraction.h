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
#ifndef USERINTERACTION_H
#define USERINTERACTION_H

#include "Utilities.h"  // It also contains the common global headers
#include "CrashReport.h"

/**
 * User interaction constants
 */
#define UI_UNDEFINED	0x00
#define	UI_OK 			0x01
#define UI_CANCEL 		0x02

/**
 * User interaction shared pointer
 */
class UserInteraction;
class AcceptInteraction;
typedef boost::shared_ptr< UserInteraction >       	UserInteractionPtr;
typedef boost::shared_ptr< AcceptInteraction >      AcceptInteractionPtr;

/**
 * User interaction callbacks
 */
typedef boost::function< void (int result) >  										callbackResult;
typedef boost::function< void (const std::string&, const callbackResult& cb) >		callbackConfirm;
typedef boost::function< void (const std::string&, const callbackResult& cb) >		callbackAlert;
typedef boost::function< void (const std::string&, const callbackResult& cb) >		callbackLicense;

/**
 * A class through interaction with the user can happen
 *
 * This class is thread-safe and allows only one pending interaction. All
 * function calls are blocking.
 */
class UserInteraction {
public:

	//////////////////////////////
	// Static functions
	//////////////////////////////

	/**
	 * Return default user interaction pointer
	 */
	static UserInteractionPtr	Default();

	/**
	 * Singleton instance of default interaction class
	 */
	static UserInteractionPtr	defaultSingleton;

	//////////////////////////////
	// Usage interface
	//////////////////////////////

	/**
	 * Constructor for user interaction class
	 */
	UserInteraction() : cbConfirm(), cbAlert(), cbLicense(), mutex(), cond() { };

	/**
	 * Display the specified message and wait for an OK/Cancel response.
	 */
	virtual int 		confirm				( const std::string & message, int timeout = 0 );

	/**
	 * Display the specified message and wait until the user clicks OK.
	 */
	virtual int 		alert				( const std::string& message, int timeout = 0 );

	/**
	 * Display a licence whose contents is fetched from the given URL and
	 * wait for user response for accepting or declining it.
	 */
	virtual int 		confirmLicense		( const std::string& url, int timeout = 0 );

	//////////////////////////////
	// Connect interface
	//////////////////////////////

	/**
	 * Define a handler for confirm message
	 */
	void 				setConfirmHandler	( const callbackConfirm & cb );

	/**
	 * Define a handler for alert message
	 */
	void 				setAlertHandler		( const callbackAlert & cb );

	/**
	 * Define a handler for alert message
	 */
	void 				setLicenseHandler	( const callbackLicense & cb );

protected:

	/**
	 * Local callbackResult function
	 */
	void				__cbResult			( int result );

	/** 
	 * Local function to wait for callback
	 */
	int 				__waitResult		( int timeout = 0 );

private:

	/**
	 * Handler for confirm message
	 */
	callbackConfirm		cbConfirm;

	/**
	 * Handler for alert message
	 */
	callbackAlert		cbAlert;

	/**
	 * Handler for alert message
	 */
	callbackLicense		cbLicense;

	/**
	 * Result value
	 */
	int 						result;

	/**
	 * Mutex and condition variable
	 */
	boost::mutex 				mutex;
	boost::condition_variable 	cond;

};

/**
 * A class that automatically accepts all user interaction 
 */
class AcceptInteraction : public UserInteraction {
public:

	/**
	 * Automatically accept a confirm message
	 */
	virtual int 		confirm				( const std::string & message, int timeout = 0 ) { return UI_OK; };

	/**
	 * Automatically accept an alert message
	 */
	virtual int 		alert				( const std::string& message, int timeout = 0 ) { return UI_OK; };

	/**
	 * Automatically confirm a license
	 */
	virtual int 		confirmLicense		( const std::string& url, int timeout = 0 ) { return UI_OK; };

};

#endif /* end of include guard: USERINTERACTION_H */
