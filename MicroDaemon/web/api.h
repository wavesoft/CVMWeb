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
#ifndef SERVERAPI_H
#define SERVERAPI_H

#include <Common/ParameterMap.h>

#include "webserver.h"
#include "api.h"

#include <json/json.h>
#include <queue>

#include <map>
#include <string>

class WebsocketAPI : public CVMWebserverConnectionHandler  {
public:

	/**
	 * Constructor for WebsocketAPI
	 */
	WebsocketAPI( const std::string& domain, const std::string& uri ) : domain(domain), url(uri), egress(), connected(true), CVMWebserverConnectionHandler() { };

	/**
	 * Virtual destructor
	 */
	virtual ~WebsocketAPI() { };

	/**
	 * This function is called when there is an incoming data frame 
	 * from the browser.
	 */
	virtual void			handleRawData( const char * buffer, const size_t len );

	/**
	 * This function should return FALSe only when the connection handler
	 * decides to drop the connection.
	 */
	virtual bool 			isConnected() { return connected; };

	/**
	 * Pops and returns the next frame from the egress queue, or returns
	 * an empty string if there are no data in the egress queue.
	 */
	virtual std::string 	getEgressRawData();

protected:

	/**
	 * The domain where the plugin resides
	 */
	std::string 			domain;

	/**
	 * The request URI
	 */
	std::string 			uri;

	/**
	 * The egress queue
	 */
	queue::< std::string >	egress;

	/**
	 * A status flag to let the server know when to drop the connection
	 */
	bool 					connected;

protected:

	/**
	 * Handle incoming actions
	 */
	virtual void			handleAction( const std::string& action, ParameterMapPtr parameters ) = 0;

	/**
	 * Send a RAW message
	 */
	void 					sendRawData( const std::string& data );

	/**
	 * Send error response
	 */
	void 					sendError( const std::string& message );

	/**
	 * Send a named action
	 */
	void 					sendAction( const std::string& name, const std::map< std::string, std::string >& data );


};

#endif /* end of include guard: SERVERAPI_H */