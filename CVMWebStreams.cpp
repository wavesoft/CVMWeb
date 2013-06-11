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

#include "CVMWebStreams.h"

/**
 * Data arrived callback -> Forward to DOWNLOAD_PROVIDER structure
 */
bool CVMWebStreamProxy::onStreamDataArrived(FB::StreamDataArrivedEvent *evt, FB::BrowserStream *) {
    size_t end = evt->getDataPosition() + evt->getLength();
    FB_UNUSED_VARIABLE(end);
    return true;
}


/**
 * Stream oppened arrived callback -> Setup length
 */
bool CVMWebStreamProxy::onStreamOpened(FB::StreamOpenedEvent *evt, FB::BrowserStream *) {
    return true;
}

/**
 * Allocate a new WebStreams provider
 */
DOWNLOAD_PROVIDER * CVMWebStreams::allocProvider() {
    DOWNLOAD_PROVIDER p = new DOWNLOAD_PROVIDER;
    
}
