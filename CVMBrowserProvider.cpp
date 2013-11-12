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

void CVMBrowserProvider::httpDataArrived ( const void * ptr, size_t length ) {
    CRASH_REPORT_BEGIN;
    CVMWA_LOG("Info", "Data arrived. Calling handler (len=" << length << ")" );
    if (this->targetStream == 0) {
        DownloadProvider::writeToStream( &this->fStream, NULL, 0, (const char*) ptr, length );
    } else {
        DownloadProvider::writeToStream( &this->sStream, NULL, 0, (const char*) ptr, length );
    }
    CRASH_REPORT_END;
}

void CVMBrowserProvider::httpProgress ( size_t current, size_t total ) {
    CRASH_REPORT_BEGIN;
    this->maxStreamSize = total;
    if (this->pf)
        DownloadProvider::fireProgressEvent( this->pf, current, total);
           
    CRASH_REPORT_END;
}

void CVMBrowserProvider::httpCompleted ( bool status, const FB::HeaderMap& headers ) {
    CRASH_REPORT_BEGIN;
    returnCode = status ? 0 : 1;
    CVMWA_LOG("Info", "Stream completed. Status : " << returnCode);
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_downloadCompleted = true;
    }
    m_cond.notify_all();
    CRASH_REPORT_END;
}

/**
 * Download a string using BrowserStreams
 */
int CVMBrowserProvider::downloadText( const std::string& url, std::string * destination, const VariableTaskPtr& pf ) {
    CRASH_REPORT_BEGIN;
    
    // Store a local pointer
    this->pf = pf;
    this->maxStreamSize = 0;
    
    // Reset timestamp
    if (pf) pf->__lastEventTime = getMillis();
        
    // Reset string stream
    CVMWA_LOG("Debug", "Resetting string stream");
    sStream.clear();
    sStream.str("");

    // Target sStream
    this->targetStream = 1;

    // Initiate asynchronous download
    {
        CVMWA_LOG("Debug", "Downloading string from '" << url << "'");
        FB::BrowserStreamRequest req(url, "GET");
        req.setCacheable(true);
        req.setSeekable( false );
        req.setBufferSize( 128*1024 );
        req.setProgressCallback( boost::bind( &CVMBrowserProvider::httpProgress, this, _1, _2 ) );
        req.setCompletedCallback( boost::bind( &CVMBrowserProvider::httpCompleted, this, _1, _2 ) );
        req.setChunkCallback( boost::bind( &CVMBrowserProvider::httpDataArrived, this, _1, _2 ) );

        // Open stream
        FB::SimpleStreamHelper::AsyncRequest( m_host, req );

        // Wait for mutex
        CVMWA_LOG("Debug", "Waiting for condition variable");
        {
            boost::unique_lock<boost::mutex> lock(m_mutex);
            m_downloadCompleted = false;
            while (!m_downloadCompleted)
                m_cond.wait(lock);
        }
        CVMWA_LOG("Debug", "Mutex released. Result = " << returnCode);

    }

    // Check if download was successful
    if (this->returnCode != 0) {
        CVMWA_LOG("Error", "BrowserStreams Error #" << returnCode );
        sStream.str("");
        return HVE_IO_ERROR;
    }

    // Copy to output and clear buffer
    *destination = sStream.str();
    sStream.str("");

    // Notify completion
    if (pf) pf->complete("Download completed");

    CVMWA_LOG("Info", "BrowserStreams download completed" );
    return HVE_OK;

    CRASH_REPORT_END;
}


/**
 * Download a file using BrowserStreams
 */
int CVMBrowserProvider::downloadFile( const std::string& url, const std::string& destination, const VariableTaskPtr& pf ) {
    CRASH_REPORT_BEGIN;
    
    // Store a local pointer
    this->pf = pf;
    this->maxStreamSize = 0;

    // Reset timestamp
    if (pf) pf->__lastEventTime = getMillis();

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
    {
        CVMWA_LOG("Debug", "Downloading string from '" << url << "'");
        FB::BrowserStreamRequest req(url, "GET");
        req.setCacheable(true);
        req.setSeekable( false );
        req.setBufferSize( 128*1024 );
        req.setProgressCallback( boost::bind( &CVMBrowserProvider::httpProgress, this, _1, _2 ) );
        req.setCompletedCallback( boost::bind( &CVMBrowserProvider::httpCompleted, this, _1, _2 ) );
        req.setChunkCallback( boost::bind( &CVMBrowserProvider::httpDataArrived, this, _1, _2 ) );

        // Open stream
        FB::SimpleStreamHelper::AsyncRequest( m_host, req );

        // Wait for mutex
        CVMWA_LOG("Debug", "Waiting for condition variable");
        {
            boost::unique_lock<boost::mutex> lock(m_mutex);
            m_downloadCompleted = false;
            while (!m_downloadCompleted)
                m_cond.wait(lock);
        }
        CVMWA_LOG("Debug", "Mutex released. Result = " << returnCode);

    }

    // Notify completion
    if (pf) pf->complete("Download completed");

    // Close stream
    fStream.close();
    return HVE_OK;
    
    CRASH_REPORT_END;
}