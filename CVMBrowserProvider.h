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

#ifndef CVMWEBSTREAMS_H_3W0U079V
#define CVMWEBSTREAMS_H_3W0U079V

#include "BrowserStream.h"
#include "BrowserHost.h"
#include "SimpleStreamHelper.h"
#include "BrowserStreamRequest.h"
#include "DefaultBrowserStreamHandler.h"

#include "Common/Hypervisor.h"
#include "Common/DownloadProvider.h"
#include "Common/Utilities.h"

/**
 * Stream handler event delegate to DOWNLOAD_PROVIDER
 */
class CVMBrowserProvider : public DownloadProvider
{
public:

    /**
     * Constructor
     */
    CVMBrowserProvider( const FB::BrowserHostPtr& host ) : 
        DownloadProvider(), m_host(host), feedbackPtr()
    {
        CVMWA_LOG("Debug", "Initializing browser provider");
    };
    
    /**
     * Destructor
     */
    virtual ~CVMBrowserProvider()
    {
        CVMWA_LOG("Debug", "Destroying browser provider");
    };

    /**
     * DefaultBrowserStreamHandler overrides
     */
    void httpDataArrived            ( const void * ptr, size_t data );
    void httpProgress               ( size_t current, size_t total );
    void httpCompleted              ( bool status, const FB::HeaderMap& headers );

    /**
     * DownloadProvider Implementation
     */
    virtual int downloadFile        ( const std::string &URL, const std::string &destination, ProgressFeedback * feedback = NULL  ) ;
    virtual int downloadText        ( const std::string &URL, std::string *buffer, ProgressFeedback * feedback = NULL );

private:
    
    FB::BrowserHostPtr				m_host;
    
    /**
     * Locking mechanism to make startDownload synchronous
     */
    boost::condition_variable       m_cond;
    boost::mutex                    m_mutex;
    bool                            m_downloadCompleted;
    int                             returnCode;

    /**
     * Progress feedback information
     */
    ProgressFeedback                * feedbackPtr;
    long                            maxStreamSize;
    std::ofstream                   fStream;
    std::ostringstream              sStream;
    short                           targetStream;


};

#endif /* end of include guard: CVMWEBSTREAMS_H_3W0U079V */
