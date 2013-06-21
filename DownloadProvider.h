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

#ifndef DOWNLOADPROVIDERS_H
#define DOWNLOADPROVIDERS_H

#include "Hypervisor.h"
#include "Utilities.h"

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
 * Throttle timer delay
 */
#define DP_THROTTLE_TIMER   250

/**
 * Callback typedef
 */
typedef boost::function<void ( const boost::shared_array<uint8_t>&, const size_t)>  callbackData;
typedef boost::function<void ( const size_t, const size_t, const std::string& )>    callbackProgress;

/**
 * Forward decleration of pointer types
 */
class _DownloadProvider; 
class CURLProvider; 
typedef boost::shared_ptr< _DownloadProvider >       DownloadProviderPtr;
typedef boost::shared_ptr< CURLProvider >            CURLProviderPtr;

/**
 * Progress feedback structure
 */
typedef struct {
    
    // Progress calculation information
    size_t              min;
    size_t              max;
    size_t              total;
    std::string         message;
    
    // Progress callback to fire
    callbackProgress    callback;

    // Used internally to throttle the event rate
    unsigned long       __lastEventTime;
    
} ProgressFeedback;

/**
 * Base class of the download provider
 */
class _DownloadProvider : public boost::enable_shared_from_this< _DownloadProvider > {
public:
    
    // Constructor & Destructor
    _DownloadProvider()          { };
    virtual ~_DownloadProvider() { };
    
    // Public interface
    virtual int                 downloadFile( const std::string &URL, const std::string &destination, ProgressFeedback * feedback = NULL   ) = 0;
    virtual int                 downloadText( const std::string &URL, std::string *buffer, ProgressFeedback * feedback = NULL ) = 0;
    
    // Get/set system default download provider
    static DownloadProviderPtr  Default();
    static void                 setDefault( const DownloadProviderPtr& provider );

};

/**
 * Interface to the CURL provider
 */
class CURLProvider : public _DownloadProvider {
public:

    // Constructor & Destructor
    CURLProvider() : _DownloadProvider(), feedbackPtr(), fStream(), sStream() {
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        }
    };
    virtual ~CURLProvider() {
        curl_easy_cleanup(curl);
    };

    // Curl I/O
    virtual int                 downloadFile( const std::string &URL, const std::string &destination, ProgressFeedback * feedback = NULL  ) ;
    virtual int                 downloadText( const std::string &URL, std::string *buffer, ProgressFeedback * feedback = NULL );

    // Private functions
    CURL                        * curl;
    ProgressFeedback            * feedbackPtr;
    long                        maxStreamSize;
    std::ofstream               fStream;
    std::ostringstream          sStream;
    
};

#endif /* end of include guard: DOWNLOADPROVIDERS_H */
