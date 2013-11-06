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
#
 include "JSAPIAuto.h"
#include "BrowserHost.h"
#
 include "CVMWeb.h"

#include "Common/Hypervisor.h"
#include "Common/CrashReport.h"

#ifndef H_CVMWebAPIDaemon
#define H_CVMWebAPIDaemon

class CVMWebAPIDaemon : public FB::JSAPIAuto
{
public:

    CVMWebAPIDaemon(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host) :
        m_plugin(plugin), m_host(host)
    {
        
        // Properties
        registerProperty("isRunning",       make_property(this, &CVMWebAPIDaemon::getIsRunning));
        registerProperty("isSystemIdle",    make_property(this, &CVMWebAPIDaemon::getIsIdle));
        registerProperty("idleTime",        make_property(this, &CVMWebAPIDaemon::getIdleTime, &CVMWebAPIDaemon::setIdleTime));
        
        // Methods
        registerMethod("start",             make_method(this, &CVMWebAPIDaemon::start));
        registerMethod("stop",              make_method(this, &CVMWebAPIDaemon::stop));
        registerMethod("check",             make_method(this, &CVMWebAPIDaemon::check));

        // Beautification
        registerMethod("toString",          make_method(this, &CVMWebAPIDaemon::toString));

        // DEBUG STUFF
        registerMethod("get",               make_method(this, &CVMWebAPIDaemon::get));
        registerMethod("set",               make_method(this, &CVMWebAPIDaemon::set));
        registerProperty("path",            make_property(this, &CVMWebAPIDaemon::getDaemonBin));
        
    }

    virtual                 ~CVMWebAPIDaemon() {};
    CVMWebPtr               getPlugin();
    
    // Methods
    int                     stop();
    int                     start();
    int                     check();
    void                    start_thread();
    
    // Read-only properties
    std::string             getDaemonBin();
    std::string             toString();
    bool                    getIsRunning();
    bool                    getIsIdle();
    
    short int               set( short int var, short int value );
    short int               get( short int var );

    // Read-Write properties    
    FB::variant             getIdleTime();
    void                    setIdleTime( short int idleTime );
    
private:
    CVMWebWeakPtr           m_plugin;
    FB::BrowserHostPtr      m_host;
    
    std::string             getDomainName();
    bool                    isDomainPrivileged();

};

#endif // H_CVMWebAPIDaemon

