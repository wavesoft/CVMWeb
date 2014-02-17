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

#include "api.h"
#include <sstream>

/**
 * Handle incoming raw request from the browser
 */
void WebsocketAPI::handleRawData( const char * buf, const size_t len ) {

	// Parse the incoming buffer as JSON
	Json::Value root;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( buf, buf+len, root );
	if ( !parsingSuccessful ) {
	    // report to the user the failure and their locations in the document.
		sendError("Unable to parse to JSON the incoming request.");
	    return;
	}

	// Ensure we have an action defined
	if (!root.isMember("type")) {
		sendError("Missing 'type' parameter in the incoming request.");
		return;
	}
	if (!root.isMember("name")) {
		sendError("Missing 'name' parameter in the incoming request.");
		return;
	}
	if (!root.isMember("id")) {
		sendError("Missing 'id' parameter in the incoming request.");
		return;
	}

	// Fetch type, action and ID
	std::string type = root["type"].asString();
	std::string a = root["name"].asString();
	std::string id = root["id"].asString();

	// Ensure type = action
	if (type != "action") {
		sendError("Unknown request type.");
		return;
	}

	// Translate json value to ParameterMapPtr
	ParameterMapPtr map = ParameterMap::instance();
	if (root.isMember("data"))
		map->fromJSON(root["data"]);

	// Handle action
	handleAction( id, a, map );

}

/**
 * Return the next available egress packet
 */
std::string WebsocketAPI::getEgressRawData() {

	// Return empty string if the queue is empty
	if (egress.empty())
		return "";

	// Pop first element
	std::string ans = egress.front();
	egress.pop();
	return ans;

}

/**
 * Send a raw response to the server
 */
void WebsocketAPI::sendRawData( const std::string& data ) {

	// Add data to the egress queue
	egress.push(data);

}

/**
 * Send error response
 */
void WebsocketAPI::sendError( const std::string& error, const std::string& id ) {
	// Build and send an error response
	std::ostringstream oss;
	oss << "{\"type\":\"error\",";
	if (!id.empty())
		oss << "\"id\":\"" << id << "\",";
	oss << "\"error\":\"" << error << "\"}";
	sendRawData( oss.str() );
}

/**
 * Send a json-formatted action response
 */
void WebsocketAPI::reply( const std::string& id, const std::map< std::string, std::string>& params ) {
	// Build and send an action response
	Json::FastWriter writer;
	Json::Value root, data;

	// Populate core fields
	root["type"] = "result";
	root["id"] = id;

	// Populate data
	for (std::map< std::string, std::string >::const_iterator it = params.begin(); it != params.end(); ++it) {
		data[(*it).first] = (*it).second;
	}
	root["data"] = data;

	// Compile JSON response
	sendRawData( writer.write(root) );
}

/**
 * Send a json-formatted action response
 */
void WebsocketAPI::sendEvent( const std::string& event, const std::string&id, const std::vector<std::string>& params ) {
	// Build and send an action response
	Json::FastWriter writer;
	Json::Value root, data;

	// Populate core fields
	root["type"] = "event";
	root["name"] = event;
	root["id"] = id;

	// Populate data
	for (std::vector<std::string >::const_iterator it = params.begin(); it != params.end(); ++it) {
		data.append(*it);
	}
	root["data"] = data;

	// Compile JSON response
	sendRawData( writer.write(root) );
}
