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
#include "DefaultBrowserStreamHandler.h"

#include "Utilities.h"

/**
 * Stream handler event delegate to DOWNLOAD_PROVIDER
 */
class CVMBrowserProvider : 
    public FB::DefaultBrowserStreamHandler, public DownloadProvider
    
{
public:

    /**
     * Constructor
     */
    CVMBrowserProvider( FB::BrowserHostPtr host ) : 
        DefaultBrowserStreamHandler(), DownloadProvider()
    {
        this->m_host = host;
        MUTEX_SETUP(this->m_mutex);
    };
    
    /**
     * Destructor
     */
    virtual ~CVMBrowserProvider()
    {
        MUTEX_CLEANUP(this->m_mutex);
    };

    /**
     * DefaultBrowserStreamHandler Event handlers
     */
    virtual bool                    onStreamDataArrived (FB::StreamDataArrivedEvent *evt, FB::BrowserStream *);
    virtual bool                    onStreamCompleted   (FB::StreamCompletedEvent *evt, FB::BrowserStream *);
    virtual bool                    onStreamOpened      (FB::StreamOpenedEvent *evt, FB::BrowserStream *);
    virtual bool                    onStreamFailedOpen  (FB::StreamFailedOpenEvent *evt, FB::BrowserStream *);

    /**
     * DownloadProvider Implementation
     */
    virtual int                     startDownload( std::string url );
    
private:
    FB::BrowserHostPtr              m_host;
    MUTEX_TYPE                      m_mutex;
    int                             returnCode;

};


#endif /* end of include guard: CVMWEBSTREAMS_H_3W0U079V */
