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

#include "SimpleStreams.h"
#include "BrowserStream.h"
#include "BrowserHost.h"
#include "DefaultBrowserStreamHandler.h"

#include "Utilities.h"

class CVMWebStreamProxy : public FB::DefaultBrowserStreamHandler 
{
public:

    /**
     * Constructor
     */
    CVMWebStreamProxy( BrowserHostPtr host, DOWNLOAD_PROVIDER * provider ) : 
        DefaultBrowserStreamHandler()
    {
        this->m_host = host;
        this->m_provider = provider;
    };

    /**
     * Event handlers
     */
    virtual bool                    onStreamDataArrived (FB::StreamDataArrivedEvent *evt, FB::BrowserStream *);
    virtual bool                    onStreamOpened      (FB::StreamOpenedEvent *evt, FB::BrowserStream *);

private:
    BrowserHostPtr                  m_host;
    DOWNLOAD_PROVIDER               m_provider;

};

/**
 * A proxy class that forwards the events from a BrowserStream to a
 * DOWNLOAD_PROVIDER structure, in order to be compatible with the
 * download(File|Text)Ex( ) functions.
 */
class CVMWebStreams 
{
public:
    
    /**
     * Constructor
     */
    CVMWebStreams( BrowserHostPtr host ) : 
        DefaultBrowserStreamHandler()
    {
        this->m_host = host;
    };
    
    /**
     * Allocate a new CVMWebStream provider
     */
    DOWNLOAD_PROVIDER *             allocProvider       ( );
    
private:

    /**
     * Callback function for the DOWNLOAD_PROVIDER struct
     */
    int                             __downloadTrigger     ( DOWNLOAD_PROVIDER * p, std::string url );

    /**
     * Local instance of the browser host
     */
    BrowserHostPtr                  m_host;
    
};

#endif /* end of include guard: CVMWEBSTREAMS_H_3W0U079V */
