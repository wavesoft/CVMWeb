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

/**
 * Javascript JSAPI adapter to control the inactivity daemon
 */
 
#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include "CVMWeb.h"
#include "Hypervisor.h"

#ifndef H_CVMWebAPIDaemon
#define H_CVMWebAPIDaemon

class CVMWebAPIDaemon : public FB::JSAPIAuto
{
public:

    CVMWebAPIDaemon(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host) :
        m_plugin(plugin), m_host(host)
    {
        
        // Properties
        registerProperty("path",            make_property(this, &CVMWebAPIDaemon::getDaemonBin));
        registerProperty("status",          make_property(this, &CVMWebAPIDaemon::getStatus));
        
        // Methods
        registerMethod("start",             make_method(this, &CVMWebAPIDaemon::start));
        registerMethod("stop",              make_method(this, &CVMWebAPIDaemon::stop));
        registerMethod("getIdleTime",       make_method(this, &CVMWebAPIDaemon::getIdleTime));
        registerMethod("setIdleTime",       make_method(this, &CVMWebAPIDaemon::setIdleTime));
        registerMethod("getIdleStatus",     make_method(this, &CVMWebAPIDaemon::getIdleStatus));
        
        // Beautification
        registerMethod("toString",          make_method(this, &CVMWebAPIDaemon::toString));
        
    }

    virtual                 ~CVMWebAPIDaemon() {};
    CVMWebPtr               getPlugin();
    
    // Methods
    int                     stop();
    int                     start();
    void                    start_thread();
    
    // Read-only properties
    std::string             getDaemonBin();
    std::string             toString();
    int                     getStatus();
    int                     getIdleStatus();

    // Read-Write properties    
    int                     getIdleTime();
    void                    setIdleTime( int idleTime );
    
private:
    CVMWebWeakPtr           m_plugin;
    FB::BrowserHostPtr      m_host;

};

#endif // H_CVMWebAPIDaemon

