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
    CVMWebPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}

/**
 * Return the current domain name
 */
std::string CVMWebAPIDaemon::getDomainName() {
    FB::URI loc = FB::URI::fromString(m_host->getDOMWindow()->getLocation());
    return loc.domain;
};

/**
 * Check if the current domain is priviledged
 */
bool CVMWebAPIDaemon::isDomainPriviledged() {
    std::string domain = this->getDomainName();
    
    /* Domain is empty only when we see the plugin from a file:// URL
     * (And yes, even localhost is not considered priviledged) */
    return domain.empty();
}

/**
 * Scripting beautification
 */
std::string CVMWebAPIDaemon::toString() {
    return "[CVMWebAPIDaemon]";
}

/**
 * Get the path of the daemon process
 */
std::string CVMWebAPIDaemon::getDaemonBin() {
    if (!isDomainPriviledged()) return "";
    
    CVMWebPtr p = this->getPlugin();
    
    /* Get the data folder location */
    std::string dPath = p->getDataFolderPath();
    
    /* Pick a daemon name according to platform */
    #ifdef _WIN32
    dPath += "/daemon/CVMWADaemon.exe";
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    dPath += "/daemon/CVMWADaemonOSX";
    #endif
    #ifdef __linux__
        #if defined(__LP64__) || defined(_LP64)
            dPath += "/daemon/CVMWADaemonLinux64";
        #else
            dPath += "/daemon/CVMWADaemonLinux";
        #endif
    #endif
    
    /* Return it */
    return dPath;
}

/**
 * Return the daemon status
 */
bool CVMWebAPIDaemon::getIsRunning() {
    
    /* Check if the daemon is running */
    std::string dLockfile = getDaemonLockfile( this->getDaemonBin() );
    return isDaemonRunning( dLockfile );
    
}

/**
 * Start the daemon in the background
 */
int startDaemon( char * path_to_bin ) {
    #ifdef _WIN32
        HINSTANCE hApp = ShellExecuteA(
            NULL,
            NULL,
            path_to_bin,
            NULL,
            NULL,
            SW_HIDE);
        if ( (int)hApp > 32 ) {
            return HVE_SCHEDULED;
        } else {
            return HVE_IO_ERROR;
        }
    #else
        int pid = fork();
        if (pid==0) {
            setsid();
            execl( path_to_bin, path_to_bin, (char*) 0 );
            return 0;
        } else {
            return HVE_SCHEDULED;
        }
    #endif
    
};

/**
 * Start daemon if it's not already running
 */
int CVMWebAPIDaemon::start() {

    /* Ensure the website is allowed to do so */
    if (!isDomainPriviledged()) return HVE_NOT_ALLOWED;
    
    /* Check if it's running */
    if (getIsRunning()) return HVE_OK;
    
    /* Get the daemon path */
    char buf[1024];
    std::string binDaemon = this->getDaemonBin();
    size_t strLen = binDaemon.copy(buf, 1024, 0);
    buf[strLen] = '\0';
    
    /* Start daemon thread */
    startDaemon( buf );
    return HVE_SCHEDULED;
    
}

/**
 * Stop daemon using the appropriate IPC message
 */
int CVMWebAPIDaemon::stop() {

    /* Ensure the website is allowed to do so */
    if (!isDomainPriviledged()) return HVE_NOT_ALLOWED;

    return daemonGet( DIPC_SHUTDOWN );
}

/**
 * Query daemon to get the current idle time settings
 */
FB::variant CVMWebAPIDaemon::getIdleTime() {
    short int idleTime = daemonGet( DIPC_GET_IDLETIME );
    return idleTime;
}

/**
 * Query daemon to set the current idle time settings
 */
void CVMWebAPIDaemon::setIdleTime( short int idleTime ) {

    /* Ensure the website is allowed to do so */
    if (!isDomainPriviledged()) return;

    daemonSet( DIPC_SET_IDLETIME, idleTime );
}

/**
 * Query daemon to get the current idle status
 */
bool CVMWebAPIDaemon::getIsIdle( ) {
    short int idleStatus = daemonGet( DIPC_IDLESTATE );
    return ( idleStatus == 1 );
}

short int CVMWebAPIDaemon::set( short int var, short int value ) {

    /* Ensure the website is allowed to do so */
    if (!isDomainPriviledged()) return HVE_NOT_ALLOWED;

    return daemonSet( var, value );
}

short int CVMWebAPIDaemon::get( short int var ) {

    /* Ensure the website is allowed to do so */
    if (!isDomainPriviledged()) return HVE_NOT_ALLOWED;

    return daemonGet( var );
}
