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

/**
 *
 */
void WebsocketAPI::handleRawData( const char * buf, const size_t len ) {

	// Parse the incoming buffer as JSON
	Json::Value root;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( buf, buf+len, root );
	if ( !parsingSuccessful ) {
	    // report to the user the failure and their locations in the document.
		sendRawData( "{\"result\":\"error\",\"error\":\"\"}" );
	    return;
	}

	// Handle data
	this->handleMessage( root );

}

/**
 *
 */
std::string WebsocketAPI::getEgressRawData() {

	// Return empty string if the queue is empty
	if (egress.empty())
		return "";

	// Pop first element
	return egress.front();
	egress.pop();

}

/**
 *
 */
void WebsocketAPI::sendRawData( const std::string& data ) {

	// Add data to the egress queue
	egress.push(data);

}