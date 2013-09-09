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

/**********************************************************\

  Auto-generated CVMWebAPIDaemon.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "DOM/Window.h"
#include "URI.h"
#include "global/config.h"

#include "DaemonCtl.h"
#include "Hypervisor.h"
#include "CVMWebAPIDaemon.h"
#include <boost/thread.hpp>

#include <sstream>

///////////////////////////////////////////////////////////////////////////////
/// @fn CVMWebPtr CVMWebAPIDaemon::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
CVMWebPtr CVMWebAPIDaemon::getPlugin()
{
    CRASH_REPORT_BEGIN;
    CVMWebPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
    CRASH_REPORT_END;
}

/**
 * Return the current domain name
 */
std::string CVMWebAPIDaemon::getDomainName() {
    CRASH_REPORT_BEGIN;
    FB::URI loc = FB::URI::fromString(m_host->getDOMWindow()->getLocation());
    return loc.domain;
    CRASH_REPORT_END;
};

/**
 * Check if the current domain is priviledged
 */
bool CVMWebAPIDaemon::isDomainPrivileged() {
    CRASH_REPORT_BEGIN;
    std::string domain = this->getDomainName();
    
    /* Domain is empty only when we see the plugin from a file:// URL
     * (And yes, even localhost is not considered priviledged) */
    return domain.empty();
    CRASH_REPORT_END;
}

/**
 * Scripting beautification
 */
std::string CVMWebAPIDaemon::toString() {
    CRASH_REPORT_BEGIN;
    return "[CVMWebAPIDaemon]";
    CRASH_REPORT_END;
}

/**
 * Get the path of the daemon process
 */
std::string CVMWebAPIDaemon::getDaemonBin() {
    CRASH_REPORT_BEGIN;
    if (!isDomainPrivileged()) return "";
    
    /* Get daemon bin from the plugin */
    CVMWebPtr p = this->getPlugin();
    return p->getDaemonBin();
    
    CRASH_REPORT_END;
}

/**
 * Return the daemon status
 */
bool CVMWebAPIDaemon::getIsRunning() {
    CRASH_REPORT_BEGIN;
    
    /* Check if the daemon is running */
    return isDaemonRunning();
    
    CRASH_REPORT_END;
}

/**
 * Start daemon if it's not already running
 */
int CVMWebAPIDaemon::start() {
    CRASH_REPORT_BEGIN;

    /* Ensure the website is allowed to do so */
    if (!isDomainPrivileged()) return HVE_NOT_ALLOWED;
    
    /* Check if it's running */
    if (getIsRunning()) return HVE_OK;
    
    /* Start daemon thread */
    daemonStart( this->getDaemonBin() );
    return HVE_SCHEDULED;
    
    CRASH_REPORT_END;
}

/**
 * Stop daemon using the appropriate IPC message
 */
int CVMWebAPIDaemon::stop() {
    CRASH_REPORT_BEGIN;

    /* Ensure the website is allowed to do so */
    if (!isDomainPrivileged()) return HVE_NOT_ALLOWED;
    return daemonStop();
    
    CRASH_REPORT_END;
}

/**
 * Query daemon to get the current idle time settings
 */
FB::variant CVMWebAPIDaemon::getIdleTime() {
    CRASH_REPORT_BEGIN;
    short int idleTime = daemonGet( DIPC_GET_IDLETIME );
    return idleTime;
    CRASH_REPORT_END;
}

/**
 * Query daemon to set the current idle time settings
 */
void CVMWebAPIDaemon::setIdleTime( short int idleTime ) {
    CRASH_REPORT_BEGIN;

    /* Ensure the website is allowed to do so */
    if (!isDomainPrivileged()) return;

    daemonSet( DIPC_SET_IDLETIME, idleTime );
    CRASH_REPORT_END;
}

/**
 * Query daemon to get the current idle status
 */
bool CVMWebAPIDaemon::getIsIdle( ) {
    CRASH_REPORT_BEGIN;
    short int idleStatus = daemonGet( DIPC_IDLESTATE );
    return ( idleStatus == 1 );
    CRASH_REPORT_END;
}

short int CVMWebAPIDaemon::set( short int var, short int value ) {
    CRASH_REPORT_BEGIN;

    /* Ensure the website is allowed to do so */
    if (!isDomainPrivileged()) return HVE_NOT_ALLOWED;

    return daemonSet( var, value );
    CRASH_REPORT_END;
}

short int CVMWebAPIDaemon::get( short int var ) {
    CRASH_REPORT_BEGIN;

    /* Ensure the website is allowed to do so */
    if (!isDomainPrivileged()) return HVE_NOT_ALLOWED;

    return daemonGet( var );
    CRASH_REPORT_END;
}

/**
 * Check if we need a daemon update
 */
int CVMWebAPIDaemon::check() {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) return p->hv->checkDaemonNeed();
    return HVE_NOT_SUPPORTED;
    CRASH_REPORT_END;
}