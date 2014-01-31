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

#include "webserver.h"
#include "api.h"

#include <json/json.h>
#include <queue>

class WebsocketAPI : public CVMWebserverConnectionHandler  {
public:

	/**
	 * Constructor for WebsocketAPI
	 */
	WebsocketAPI( const std::string& domain ) : domain(domain), egress(), connected(true) { };

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

private:

	/**
	 * The domain where the plugin resides
	 */
	std::string 			domain;

	/**
	 * The egress queue
	 */
	queue::< std::string >	egress;

	/**
	 * A status flag to let the server know when to drop the connection
	 */
	bool 					connected;

	/**
	 * Send a RAW message
	 */
	void 					sendRawData( const std::string& data );

private:

	/**
	 * Handle incoming actions
	 */

};

#endif /* end of include guard: SERVERAPI_H */