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

#include <Common/ParameterMap.h>

/**
 * Translate json value to parameter map
 */
void parseJSONtoParameters( const Json::Value& json, ParameterMapPtr map ) {
	const Json::Value::Members membNames = json.getMemberNames();
	for (std::vector<std::string>::begin it = membNames.begin(); it != membNames.end(); ++it) {
		std::string k = *it;
		Json::Value v = json[k];

		if (v.isObject()) {
			parseJSONtoParameters(v, map->subgroup(k));
		} else {
			map->set(k, v.asString());
		}

	}
}

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
	if (!root.isMember("action")) {
		sendError("Missing action parameter in the incoming request.");
		return;
	}

	// Fetch action
	std::string a = root["action"].asString();

	// Translate json value to ParameterMapPtr
	ParameterMapPtr map = ParameterMap::instance();
	parseJSONtoParameters( root, map );

	// Handle action
	handleAction(a, map );

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
void WebsocketAPI::sendError( const std::string& error ) {
	// Build and send an error response
	std::ostringstream oss;
	oss << "{\"result\":\"error\",\"error\":\"" << error << "\"}";
	sendRawData( oss.str() );
}

/**
 * Send a json-formatted action response
 */
void WebsocketAPI::sendAction( const std::string& action, const std::map< std::string, std::string >& data ) {
	// Build and send an action response
	Json::FastWriter writer;
	Json::Value root, data;

	// Populate core fields
	root["result"] = "ok";
	root["action"] = action;

	// Populate data
	for (std::map< std::string, std::string >::const_iterator it = data.begin(); it != data.end(); ++it) {
		data[(*it).first] = (*it).second();
	}
	root["data"] = data;

	// Compile JSON response
	sendRawData( writer.write(root) );
}
