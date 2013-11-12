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
#ifndef DOWNLOADPROVIDERS_H
#define DOWNLOADPROVIDERS_H

#include "Utilities.h"
#include "ProgressFeedback.h"
#include "CrashReport.h"

#include <ostream>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <curl/curl.h>
#include <curl/easy.h>

/**
 * Throttle timer delay which defines how frequetly the progress events will be fired
 */
#define DP_THROTTLE_TIMER   250

/**
 * Forward decleration of pointer types
 */
class DownloadProvider; 
class CURLProvider; 
typedef boost::shared_ptr< DownloadProvider >       DownloadProviderPtr;
typedef boost::shared_ptr< CURLProvider >           CURLProviderPtr;

/**
 * Base class of the download provider
 */
class DownloadProvider {
public:
    
    // Constructor & Destructor
    DownloadProvider()          { };
    virtual ~DownloadProvider() { };
    
    // Public interface
    virtual int                 downloadFile( const std::string &URL, const std::string &destination, const VariableTaskPtr& pf = VariableTaskPtr() ) = 0;
    virtual int                 downloadText( const std::string &URL, std::string *buffer, const VariableTaskPtr& pf = VariableTaskPtr() ) = 0;
    
    // Get/set system default download provider
    static DownloadProviderPtr  Default();
    static void                 setDefault( const DownloadProviderPtr& provider );

    // Helper functions
    static void                 fireProgressEvent( const VariableTaskPtr& pf, size_t pos, size_t max );
    static void                 writeToStream( std::ostream * stream, const VariableTaskPtr& pf, long max_size, const char * ptr, size_t data );

};

/**
 * Interface to the CURL provider
 */
class CURLProvider : public DownloadProvider {
public:

    // Constructor & Destructor
    CURLProvider() : DownloadProvider(), pf(), fStream(), sStream() {
        
        // Initialize curl
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        }

        // Reset vars
        this->maxStreamSize = 0;

    };
    virtual ~CURLProvider() {
        curl_easy_cleanup(curl);
    };

    // Curl I/O
    virtual int                 downloadFile( const std::string &URL, const std::string &destination, const VariableTaskPtr& pf = VariableTaskPtr()  ) ;
    virtual int                 downloadText( const std::string &URL, std::string *buffer, const VariableTaskPtr& pf = VariableTaskPtr() );

    // Private functions
    CURL                        * curl;
    VariableTaskPtr             pf;
    long                        maxStreamSize;
    std::ofstream               fStream;
    std::ostringstream          sStream;
    
};

#endif /* end of include guard: DOWNLOADPROVIDERS_H */
