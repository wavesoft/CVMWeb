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
#ifndef DAEMON_SESSION_H
#define DAEMON_SESSION_H

#include <Common/Config.h>
#include <Common/UserInteraction.h>

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include "web/webserver.h"
#include "web/api.h"

#include "daemon_core.h"


/**
 * Websocket Session
 */
class DaemonSession : public WebsocketAPI {
public:

	/**
	 * Constructor
	 */
	DaemonSession( const std::string& domain, const std::string uri, DaemonCore& core )
		: WebsocketAPI(domain, uri), core(core), privileged(false), userInteraction(), interactionCallback()
	{

		// Prepare user interaction
		userInteraction = boost::make_shared<UserInteraction>();
		userInteraction->setConfirmHandler( boost::bind( &DaemonSession::__callbackConfim, this, _1, _2, _3 ) );
		userInteraction->setAlertHandler( boost::bind( &DaemonSession::__callbackAlert, this, _1, _2, _3 ) );
		userInteraction->setLicenseHandler( boost::bind( &DaemonSession::__callbackLicense, this, _1, _2, _3 ) );
		userInteraction->setLicenseURLHandler( boost::bind( &DaemonSession::__callbackLicenseURL, this, _1, _2, _3 ) );

		// Reset throttling parameters
	    throttleTimestamp = 0;
	    throttleDenies = 0;
	    throttleBlock = false;
	};

protected:

	/**
	 * API actino handler
	 */
	virtual void handleAction( const std::string& id, const std::string& action, ParameterMapPtr parameters );

	/**
	 * The daemon core instance
	 */
	DaemonCore&	core;

	/**
	 * The User interaction class
	 */
	UserInteractionPtr	userInteraction;

	/**
	 * A flag that defines if this session is authenticated
	 * for privileged operations
	 */
	bool 	privileged;

    // Throttling protection
    long	throttleTimestamp;
    int 	throttleDenies;
    bool 	throttleBlock;

private:

	/**
	 * Delegate function that forwards the request to the javascript interface
	 */
	void __callbackConfim		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackAlert		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackLicense		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackLicenseURL	(const std::string&, const std::string&, const callbackResult& cb);

	/**
	 * The user interaction callback where we should forward the responses
	 */
	callbackResult interactionCallback;

	/**
	 * RequestSession Thread
	 */
	void requestSession_thread( const std::string& eventID, const std::string& vmcpURL );

};

#endif /* end of include guard: DAEMON_SESSION_H */