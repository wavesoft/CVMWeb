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

#include "CVMBrowserProvider.h"

/**
 * Data arrived callback -> Forward to DOWNLOAD_PROVIDER structure
 */
bool CVMBrowserProvider::onStreamDataArrived( FB::StreamDataArrivedEvent *evt, FB::BrowserStream * ) {
    this->callDataHandler( (void*)evt->getData(), evt->getLength() );
    return true;
}

/**
 * Stream oppened arrived callback -> Setup length
 */
bool CVMBrowserProvider::onStreamOpened( FB::StreamOpenedEvent *evt, FB::BrowserStream * s) {
    this->dataLength = s->getLength();
    return true; 
}

/**
 * Notify then download trigger when the file is downloaded
 */
bool CVMBrowserProvider::onStreamCompleted(FB::StreamCompletedEvent *evt, FB::BrowserStream *) {
    this->returnCode = 0;
    MUTEX_UNLOCK(m_mutex);
    return true;
}

/**
 * Notify the download trigger if something failed
 */
bool CVMBrowserProvider::onStreamFailedOpen(FB::StreamFailedOpenEvent *evt, FB::BrowserStream *) {
    this->returnCode = -1;
    MUTEX_UNLOCK(m_mutex);
    return true;
}

/**
 * Allocate a new WebStreams provider
 */
int CVMBrowserProvider::startDownload( std::string url ) {
    
    /**
     * Start stream
     */
    FB::BrowserStreamPtr tempStream = m_host->createStream( url, (FB::PluginEventSinkPtr) this, true, false );
    FB_UNUSED_VARIABLE(tempStream);

    /**
     * Block until it's downloaded
     */
    MUTEX_LOCK(m_mutex);
    return returnCode;
}
