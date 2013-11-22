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
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <map>
#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Utilities.h"

// Include firebreath headers only if we are building for firebrath
#ifdef FIREBRATH_BUILD
#include "JSObject.h"
#include "variant_list.h"
#endif

//////////////////////////////////////
// Callback function declerations
//////////////////////////////////////

/* Callback functions reference */
typedef boost::function<void ( const std::string& message )>    						cbStarted;
typedef boost::function<void ( const std::string& message )>    						cbCompleted;
typedef boost::function<void ( const std::string& message, const int errorCode )> 		cbFailed;
typedef boost::function<void ( const std::string& message, const double progress )> 	cbProgress;

//////////////////////////////////////
// Classes and structures
//////////////////////////////////////

/**
 * The CallbackHost class provides the interface to register and fire
 * callbacks by name.
 */
class CallbackHost {
public:

	CallbackHost() : startedCallbacks(), completedCallbacks(), failedCallbacks(), progressCallbacks() { };

	/**
	 * Register a callback that will be fired when the very first task has
	 * started progressing.
	 */
	void 				onStarted		( const cbStarted & cb );

	/**
	 * Register a callback that will be fired when the last task has completed
	 * progress.
	 */
	void 				onCompleted		( const cbCompleted & cb );

	/**
	 * Register a callback that will be fired when an error has occured.
	 */
	void 				onFailed		( const cbFailed & cb );

	/**
	 * Register a callback event that will be fired when a progress
	 * event is updated.
	 */
	void 				onProgress		( const cbProgress & cb );

	/**
	 * Register a named callback handler
	 */

	// Register & Call 'started' event
	void 				fireStarted( const std::string & msg );
	void 				fireCompleted( const std::string & msg );
	void 				fireFailed( const std::string & msg, const int errorCode );
	void 				fireProgress( const std::string& msg, const double progress );

public:

	// Callback list
	std::vector< cbStarted >		startedCallbacks;
	std::vector< cbCompleted >		completedCallbacks;
	std::vector< cbFailed >			failedCallbacks;
	std::vector< cbProgress >		progressCallbacks;

};


// Build the firebreath interface only if we are building for firebrath
#ifdef FIREBRATH_BUILD

/**
 * Wrapper class that registers handlers for javascript events
 */
class FBCallbackHost {
public:

	FBCallbackHost( const CallbackHostPtr & ch ) : parent(ch) 	{ };

	void 				jsStarted( const FB::variant &cb )		{ cbStarted = cb };
	void				jsCompleted( const FB::variant &cb )	{ cbCompleted = cb };
	void 				jsFailed( const FB::variant &cb )		{ cbFailed = cb };
	void 				jsProgress( const FB::variant &cb )		{ cbProgress = cb };

private:

	const CallbackHostPtr parent;
	FB::variant 		jcbStarted;
	FB::variant 		jcbCompleted;
	FB::variant 		jcbFailed;
	FB::variant 		jcbProgress;

	void 				_delegate_jsStarted( const std::string & msg );
	void 				_delegate_jsCompleted( const std::string & msg );
	void 				_delegate_jsFailed( const std::string & msg, const int errorCode );
	void 				_delegate_jsProgress( const std::string& msg, const double progress );

};
#endif

#endif /* end of include guard: CALLBACKS_H */