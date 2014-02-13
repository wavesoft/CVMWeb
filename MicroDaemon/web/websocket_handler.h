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
#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <mongoose.h>
#include <json/json.h>

#include <string>
#include <queue>
#include <map>

/**
 * An active WebSocket connection on the webserver
 */
class CVMWebsocketHandler {
public:

	/////////////////////////////////////////////
	// Constructor & Destructor
	/////////////////////////////////////////////


	/**
	 * This class is constructed when a websocket is oppened
	 * and will be destructed when the socket is destructed;
	 */
	CVMWebsocketHandler( mg_connection *conn, const std::string& domain ) 
		: _conn(conn), _isAlive(true), _isIterated(false), domain(domain), _egress() { };

	/**
	 * The destructor is called when the web socket is disconnected
	 */
	virtual ~CVMWebsocketHandler() { };

public:

	/////////////////////////////////////////////
	// Public functions that must be available
	// for TConnectionHandler compatibility
	////////////////////////////////////////////

	/**
	 * Process incoming data
	 */
	void _handleData( char * buf, size_t len )
	{

	    // Create a string object from buffer
	    std::string buffer(buf, len);

	    // Parse the incoming buffer as JSON
	    Json::Value root;
	    Json::Reader reader;
	    bool parsingSuccessful = reader.parse( buf, buf+len, root );
	    if ( !parsingSuccessful ) {
	        // report to the user the failure and their locations in the document.
	        std::cout  << "SOCKET: Parse Error: " << reader.getFormatedErrorMessages();
	        return;
	    }

	    // Handle data
	    this->handleMessage( root );

	};

    /**
	 * A flag used for detecting disconnected socket
	 */
	bool 					_isAlive;

	/**
	 * A flag used for detecting lost connections
	 */
	bool 					_isIterated;

	/**
	 * The active mongoose server connection
	 */
	mg_connection 			*_conn;

	/**
	 * Outgoing data frames
	 */
	std::queue<std::string>	_egress;


protected:

	/////////////////////////////////////////////
	// Implementation Entry Point
	/////////////////////////////////////////////

	/**
	 * Overridable function to handle incoming data
	 */
	void handleMessage( const Json::Value& data ) = 0;

protected:

	/////////////////////////////////////////////
	// Utility functions available to subclasses
	/////////////////////////////////////////////

	/**
	 * The domain this websocket is requested from
	 */
	std::string				domain;

	/**
	 * Disconnect socket
	 */
	void close() 
	{ 
		this->_isAlive = false; 
	}

	/**
	 * Send a message
	 */
	void sendMessage( const Json::Value& data )
	{

	    // Dump JSON buffer and place on egress
	    Json::FastWriter writer;
	    _egress.push( writer.write( data ) );

	};

	/**
	 * Send raw data
	 */
	void sendData( const std::string& data )
	{

	    // Dump string buffer in egress queue
	    _egress.push( data );

	};

};

#endif /* end of include guard: WEBSOCKET_HANDLER_H */