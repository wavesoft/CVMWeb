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

#include "web/webserver.h"
#include "web/api.h"

#include "daemon_core.h"


/**
 * Websocket Session
 */
class DaemonSession : public WebsocketAPI, public UserInteraction {
public:

	/**
	 * Constructor
	 */
	DaemonSession( const std::string& domain, const std::string uri, DaemonCore& core )
		: WebsocketAPI(domain, uri), UserInteraction(), core(core), privileged(false)
	{

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
	 * RequestSession Thread
	 */
	HVSessionPtr requestSession( const std::string& id, const std::string& vmcpURL );

};

#endif /* end of include guard: DAEMON_SESSION_H */