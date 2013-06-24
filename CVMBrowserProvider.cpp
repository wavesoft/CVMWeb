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
#include <URI.h>

/**
 * Data arrived callback -> Forward to DOWNLOAD_PROVIDER structure
 */
bool CVMBrowserProvider::onStreamDataArrived( FB::StreamDataArrivedEvent *evt, FB::BrowserStream * ) {
    CVMWA_LOG("Info", "Data arrived. Calling handler (len=" << evt->getLength() << ")" );
    if (this->targetStream == 0) {
        DownloadProvider::writeToStream( &this->fStream, this->feedbackPtr, this->maxStreamSize, (const char*) evt->getData(), evt->getLength() );
    } else {
        DownloadProvider::writeToStream( &this->sStream, this->feedbackPtr, this->maxStreamSize, (const char*) evt->getData(), evt->getLength() );
    }
    return true;
}

/**
 * Stream oppened arrived callback -> Setup length
 */
bool CVMBrowserProvider::onStreamOpened( FB::StreamOpenedEvent *evt, FB::BrowserStream * s) {
    this->maxStreamSize = s->getLength();
    CVMWA_LOG("Info", "Stream open. Data length: " << this->maxStreamSize );
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
 * Download a string using BrowserStreams
 */
int CVMBrowserProvider::downloadText( const std::string& url, std::string * destination, ProgressFeedback * feedback ) {
    
    // Store a local pointer
    this->feedbackPtr = feedback;
    this->maxStreamSize = 0;
    
    // Reset timestamp on feedback
    if (feedback != NULL)
        feedback->__lastEventTime = getMillis();
        
    // Reset string stream
    CVMWA_LOG("Debug", "Resetting string stream");
    sStream.clear();
    sStream.str("");

    // Target sStream
    this->targetStream = 1;

    // Initiate asynchronous download
    CVMWA_LOG("Debug", "Downloading string from '" << url << "'");
    FB::BrowserStreamRequest req(url, "GET");
    req.setCacheable(true);

    //std::shared_ptr< FB::DefaultBrowserStreamHandler > wDerived = SmartPtrBuilder::CreateSharedPtr< Def, FB::DefaultBrowserStreamHandler >( this );

    req.setEventSink( this->DownloadProvider::shared_from_this() );
    //FB::BrowserStreamPtr stream(m_host->createStream(req, false));

    // Wait for mutex
    CVMWA_LOG("Debug", "Waiting for condition variable");
    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_cond.wait(lock);
    CVMWA_LOG("Debug", "Mutex released. Result = " << returnCode);

    // Check if download was successful
    if (this->returnCode != 0) {
        CVMWA_LOG("Error", "BrowserStreams Error #" << returnCode );
        sStream.str("");
        return HVE_IO_ERROR;
    }

    // Copy to output and clear buffer
    *destination = sStream.str();
    sStream.str("");

    CVMWA_LOG("Info", "BrowserStreams download completed" );
    return HVE_OK;

}


/**
 * Download a file using BrowserStreams
 */
int CVMBrowserProvider::downloadFile( const std::string& url, const std::string& destination, ProgressFeedback * feedback ) {
    
    // Setup CURL url
    CVMWA_LOG("Debug", "Downloading file from '" << url << "'");
    
    // Store a local pointer
    this->feedbackPtr = feedback;
    this->maxStreamSize = 0;

    // Reset timestamp on feedback
    if (feedback != NULL)
        feedback->__lastEventTime = getMillis();

    // Open local file
    CVMWA_LOG("Debug", "Oppening local output stream '" << destination << "'");
    fStream.clear();
    fStream.open( destination.c_str(), std::ofstream::binary );
    if (fStream.fail()) {
        CVMWA_LOG("Error", "OFStream error" );
        return HVE_IO_ERROR;
    }
    
    // Target sStream
    this->targetStream = 0;

    // Initiate asynchronous download
    CVMWA_LOG("Debug", "Downloading string from '" << url << "'");
    FB::BrowserStreamRequest req(url, "GET");
    req.setCacheable(true);
    req.setEventSink( this->FB::DefaultBrowserStreamHandler::shared_from_this() );
    FB::BrowserStreamPtr stream(m_host->createStream(req, false));

    // Wait for mutex
    CVMWA_LOG("Debug", "Waiting for condition variable");
    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_cond.wait(lock);
    CVMWA_LOG("Debug", "Mutex released. Result = " << returnCode);
    
    // Close stream
    fStream.close();
    return HVE_OK;
    
}