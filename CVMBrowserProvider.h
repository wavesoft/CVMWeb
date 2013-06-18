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

#include "Hypervisor.h"
#include "Utilities.h"

/**
 * Stream handler event delegate to DOWNLOAD_PROVIDER
 */
class CVMBrowserProvider : public DownloadProvider, public FB::DefaultBrowserStreamHandler
{
public:

    /**
     * Constructor
     */
    CVMBrowserProvider( const FB::BrowserHostPtr& host ) : 
        DownloadProvider(), DefaultBrowserStreamHandler(), m_host(host)
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
    bool onStreamDataArrived        ( FB::StreamDataArrivedEvent *evt, FB::BrowserStream * );
    bool onStreamOpened             ( FB::StreamOpenedEvent *evt, FB::BrowserStream * s);
    bool onStreamCompleted          (FB::StreamCompletedEvent *evt, FB::BrowserStream *);
    bool onStreamFailedOpen         (FB::StreamFailedOpenEvent *evt, FB::BrowserStream *);

    /**
     * DownloadProvider Implementation
     */
    virtual int startDownload       ( std::string url );
    
    /**
     *
     */
    //void                            getURLCallback      (bool success, const FB::HeaderMap& headers, const boost::shared_array<uint8_t>& data, const size_t size);
    
private:
    
    FB::BrowserHostPtr				m_host;
    
    /**
     * Locking mechanism to make startDownload synchronous
     */
    boost::condition_variable       m_cond;
    boost::mutex                    m_mutex;
    int                             returnCode;

};


#endif /* end of include guard: CVMWEBSTREAMS_H_3W0U079V */
