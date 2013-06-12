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
    CVMWA_LOG("Info", "Data arrived. Calling handler (len=" << evt->getLength() << ")" );
    this->callDataHandler( (void*)evt->getData(), evt->getLength() );
    return true;
}

/**
 * Stream oppened arrived callback -> Setup length
 */
bool CVMBrowserProvider::onStreamOpened( FB::StreamOpenedEvent *evt, FB::BrowserStream * s) {
    this->dataLength = s->getLength();
    CVMWA_LOG("Info", "Stream open. Data length: " << this->dataLength );
    return true; 
}

/**
 * Notify then download trigger when the file is downloaded
 */
bool CVMBrowserProvider::onStreamCompleted(FB::StreamCompletedEvent *evt, FB::BrowserStream *) {
    returnCode = 0;
    CVMWA_LOG("Info", "Stream completed");
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_cond.notify_all();
    return true;
}

/**
 * Notify the download trigger if something failed
 */
bool CVMBrowserProvider::onStreamFailedOpen(FB::StreamFailedOpenEvent *evt, FB::BrowserStream *) {
    returnCode = HVE_IO_ERROR;
    CVMWA_LOG("Error", "Failed to open stream");
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_cond.notify_all();
    return true;
}

/**
 * Allocate a new WebStreams provider
 */
int CVMBrowserProvider::startDownload( std::string url ) {
    returnCode = 0;
    
    /*
    try {
        
        // Prepare stream request
        CVMWA_LOG("Debug", "Requesting " << url);
        FB::BrowserStreamRequest req( url, "GET" );

        req.setCacheable(true);
        req.setSeekable(false);
        req.setEventSink( (FB::PluginEventSinkPtr) this );

        // Perform asynchronous request
        CVMWA_LOG("Debug", "Starting async request");
        FB::BrowserStreamPtr mStream = m_host->createStream( req );
        mStream->AttachObserver( req );

    } catch (const std::exception& e) {
        // If anything weird happens, return error
        CVMWA_LOG("Error", "An I/O exception occured! " << e.what());
        return HVE_IO_ERROR;
    }
    */
    
    /**
     * Block until it's downloaded
     */
     /*
    CVMWA_LOG("Debug", "Waiting for condition variable");
    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_cond.wait(lock);
    CVMWA_LOG("Debug", "Mutex released. Result = " << returnCode);
    return returnCode;
    */
    
    CVMWA_LOG("WAT", "What-cha-doing?");
    return returnCode;
}
