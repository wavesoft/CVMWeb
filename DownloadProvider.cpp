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

#include "DownloadProvider.h"
#include "Hypervisor.h"

DownloadProviderPtr systemProvider;

/**
 * Get system-wide download provider singleton
 */
DownloadProviderPtr DownloadProvider::Default() {
    if (!systemProvider)
        systemProvider = boost::make_shared< CURLProvider >();
    return systemProvider;
}

/**
 * Set system-wide default provider singleton
 */
void DownloadProvider::setDefault( const DownloadProviderPtr& provider ) {
    systemProvider = provider;
}

/**
 * Local function to fire the progress event accordingly
 */
void DownloadProvider::fireProgressEvent( ProgressFeedback * fb, size_t pos, size_t max ) {
    if (fb != NULL) {
        
        // Throttle events on 2 per second, unless that's the last one
        if ((pos != max) && ((getMillis() - fb->__lastEventTime) < DP_THROTTLE_TIMER)) return;
        fb->__lastEventTime = getMillis();

        // Calculate percentage
        size_t ipos = fb->min;
        if (max != 0) {
            
            double v1 = ((fb->max - fb->min) * pos);
            double v2 = v1 / max;
            
            ipos += (size_t)v2;
        }
        CVMWA_LOG("Debug", "Calling progress function with " << ipos << " in (" << fb->min << "-" << fb->max << "/" << fb->total << ")" );
        
        // Fire callback
        fb->callback( ipos, fb->total, fb->message );
        
    }
}

/**
 * Local function to write data to osstream
 */
void DownloadProvider::writeToStream( std::ostream * stream, ProgressFeedback * feedbackPtr, long max_size, const char * ptr, size_t data ) {
    
    // Write data to the file
    stream->write( ptr, data );
    
    // Update progress
    if (max_size != 0) {
        
        // Get position inside the stream
        size_t cur_size = stream->tellp();
        
        // Fire progress update
        if (feedbackPtr != NULL)
            DownloadProvider::fireProgressEvent( feedbackPtr, cur_size, max_size );
    }

}

/**
 * Extract the content-length from function
 */
size_t __curl_headerfunc( void *ptr, size_t size, size_t nmemb, CURLProvider * self) {
    size_t dataLen = size * nmemb;
    
    // Move data to std::String
    std::string cppString( (char *) ptr, dataLen );
    if ((cppString.length() > 16) &&  (cppString.substr(0,16).compare("Content-Length: ") == 0)) {
        cppString = cppString.substr(16, cppString.length()-18 );
        CVMWA_LOG("Debug", "Found Content-Length: '" << cppString << "'");
        self->maxStreamSize = ston<size_t>( cppString );
    }
    
    return dataLen;
}

/**
 * Callback function for CURL data
 */
size_t __curl_datacb_file(void *ptr, size_t size, size_t nmemb, CURLProvider * self ) {
    size_t dataLen = size * nmemb;

    //CVMWA_LOG("Debug", "cURL File callback (size=" << dataLen << ")");

    // Write to file stream
    DownloadProvider::writeToStream( &(self->fStream), self->feedbackPtr, self->maxStreamSize, (const char *) ptr, dataLen );
    
    // Return data len
    return dataLen;
}

/**
* Callback function for CURL data
 */
size_t __curl_datacb_string(void *ptr, size_t size, size_t nmemb, CURLProvider * self ) {
    size_t dataLen = size * nmemb;

    CVMWA_LOG("Debug", "cURL String callback (size=" << dataLen << ")");

    // Write to string stream
    DownloadProvider::writeToStream( &(self->sStream), self->feedbackPtr, self->maxStreamSize, (const char *) ptr, dataLen );

    // Return data len
    return dataLen;
}

/**
 * Download a file using CURL
 */
int CURLProvider::downloadFile( const std::string& url, const std::string& destination, ProgressFeedback * feedback ) {
    
    // Setup CURL url
    CVMWA_LOG("Debug", "Downloading file from '" << url << "'");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Store a local pointer
    this->feedbackPtr = feedback;
    this->maxStreamSize = 0;

    // Reset timestamp on feedback
    if (feedback != NULL)
        feedback->__lastEventTime = getMillis();
    
    // Setup callbacks
    //CURLProviderPtr sharedPtr = boost::dynamic_pointer_cast< CURLProvider >( shared_from_this() );
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __curl_headerfunc);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __curl_datacb_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this); //sharedPtr.get() );
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this); //sharedPtr.get() );
    
    // Open local file
    CVMWA_LOG("Debug", "Oppening local output stream '" << destination << "'");
    fStream.clear();
    fStream.open( destination.c_str(), std::ofstream::binary );
    if (fStream.fail()) {
        CVMWA_LOG("Error", "OFStream error" );
        return HVE_IO_ERROR;
    }
    
    // Initiate connection (we have specified CURLOPT_CONNECT_ONLY)
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        CVMWA_LOG("Error", "cURL Error #" << res );
        return HVE_IO_ERROR;
    } else {
        CVMWA_LOG("Info", "cURL Download completed" );
    }
    
    // Close stream
    fStream.close();
    return HVE_OK;
    
}

/**
 * Download a file using CURL
 */
int CURLProvider::downloadText( const std::string& url, std::string * destination, ProgressFeedback * feedback ) {
    
    // Setup CURL url
    CVMWA_LOG("Debug", "Downloading string from '" << url << "'");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Store a local pointer
    this->feedbackPtr = feedback;
    this->maxStreamSize = 0;
    
    // Reset timestamp on feedback
    if (feedback != NULL)
        feedback->__lastEventTime = getMillis();
    
    // Setup callbacks
    //CURLProviderPtr sharedPtr = boost::dynamic_pointer_cast< CURLProvider >( shared_from_this() );
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __curl_headerfunc);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __curl_datacb_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this); //sharedPtr.get() );
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this); //sharedPtr.get() );
    
    // Reset string stream
    CVMWA_LOG("Debug", "Resetting string stream");
    sStream.clear();
    sStream.str("");
    
    // Initiate connection (we have specified CURLOPT_CONNECT_ONLY)
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        CVMWA_LOG("Error", "cURL Error #" << res );
        sStream.str("");
        return HVE_IO_ERROR;
    }
    
    // Copy to output and clear buffer
    *destination = sStream.str();
    sStream.str("");

    CVMWA_LOG("Info", "cURL Download completed" );
    return HVE_OK;
    
}
