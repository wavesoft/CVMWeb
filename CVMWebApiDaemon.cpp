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

std::string CVMWebAPIDaemon::toString() {
    return "[CVMWebAPIDaemon]";
}

/**
 * Get the path of the daemon process
 */
std::string CVMWebAPIDaemon::getDaemonBin() {
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
    dPath += "/daemon/CVMWADaemonLinux";
    #endif
    
    /* Return it */
    return dPath;
}

/**
 * Return the daemon status
 */
int CVMWebAPIDaemon::getStatus() {
    
    /* Check if the daemon is running */
    std::string dLockfile = getDaemonLockfile( this->getDaemonBin() );
    if (isDaemonRunning( dLockfile )) {
        return 1;
    } else {
        return 0;
    }
    
}

int startDaemon( char * path_to_bin ) {
    #ifdef _WIN32
    
    #else
    int pid = fork();
    if (pid==0) {
        setsid();
        execl( path_to_bin, path_to_bin, (char*) 0 );
        return 0;
    } else {
        return HVE_SHEDULED;
    }
    #endif
    
};

/**
 * Start daemon if it's not already running
 */
int CVMWebAPIDaemon::start() {
    
    /* Check if it's running */
    if (getStatus() == 1) return HVE_OK;
    
    /* Get the daemon path */
    char buf[1024];
    std::string binDaemon = this->getDaemonBin();
    size_t strLen = binDaemon.copy(buf, 1024, 0);
    buf[strLen] = '\0';
    
    /* Start daemon thread */
    startDaemon( buf );
    return HVE_SHEDULED;
    
}

/**
 * Stop daemon using the appropriate IPC message
 */
int CVMWebAPIDaemon::stop() {
    return daemonGet( DIPC_SHUTDOWN );
}

/**
 * Query daemon to get the current idle time settings
 */
FB::variant CVMWebAPIDaemon::getIdleTime() {
    long int idleTime = daemonGet( DIPC_GET_IDLETIME );
    return idleTime;
}

/**
 * Query daemon to set the current idle time settings
 */
void CVMWebAPIDaemon::setIdleTime( long int idleTime ) {
    daemonSet( DIPC_SET_IDLETIME, idleTime );
}

/**
 * Query daemon to get the current idle status
 */
bool CVMWebAPIDaemon::getIdleStatus( ) {
    long int idleStatus = daemonGet( DIPC_IDLESTATE );
    return ( idleStatus == 1 );
}