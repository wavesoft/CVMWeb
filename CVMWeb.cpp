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

  Auto-generated CVMWeb.cpp

  This file contains the auto-generated main plugin object
  implementation for the CernVM Web API project

\**********************************************************/

#include "CVMWeb.h"
#include "CVMWebAPI.h"

#include "Hypervisor.h"
#include "ThinIPC.h"

///////////////////////////////////////////////////////////////////////////////
/// @fn CVMWeb::StaticInitialize()
///
/// @brief  Called from PluginFactory::globalPluginInitialize()
///
/// @see FB::FactoryBase::globalPluginInitialize
///////////////////////////////////////////////////////////////////////////////
void CVMWeb::StaticInitialize()
{
    // Place one-time initialization stuff here; As of FireBreath 1.4 this should only
    // be called once per process
    thinIPCInitialize();
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CVMWeb::StaticInitialize()
///
/// @brief  Called from PluginFactory::globalPluginDeinitialize()
///
/// @see FB::FactoryBase::globalPluginDeinitialize
///////////////////////////////////////////////////////////////////////////////
void CVMWeb::StaticDeinitialize()
{
    // Place one-time deinitialization stuff here. As of FireBreath 1.4 this should
    // always be called just before the plugin library is unloaded
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  CVMWeb constructor.  Note that your API is not available
///         at this point, nor the window.  For best results wait to use
///         the JSAPI object until the onPluginReady method is called
///////////////////////////////////////////////////////////////////////////////
CVMWeb::CVMWeb()
{
    // Detect the hypervisor the user has installed
    this->hv = detectHypervisor();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  CVMWeb destructor.
///////////////////////////////////////////////////////////////////////////////
CVMWeb::~CVMWeb()
{
    // This is optional, but if you reset m_api (the shared_ptr to your JSAPI
    // root object) and tell the host to free the retained JSAPI objects then
    // unless you are holding another shared_ptr reference to your JSAPI object
    // they will be released here.
    releaseRootJSAPI();
    m_host->freeRetainedObjects();
}

void CVMWeb::onPluginReady()
{
    // When this is called, the BrowserHost is attached, the JSAPI object is
    // created, and we are ready to interact with the page and such.  The
    // PluginWindow may or may not have already fire the AttachedEvent at
    // this point.
}

void CVMWeb::shutdown()
{
    // This will be called when it is time for the plugin to shut down;
    // any threads or anything else that may hold a shared_ptr to this
    // object should be released here so that this object can be safely
    // destroyed. This is the last point that shared_from_this and weak_ptr
    // references to this object will be valid
}

///////////////////////////////////////////////////////////////////////////////
/// @brief  Creates an instance of the JSAPI object that provides your main
///         Javascript interface.
///
/// Note that m_host is your BrowserHost and shared_ptr returns a
/// FB::PluginCorePtr, which can be used to provide a
/// boost::weak_ptr<CVMWeb> for your JSAPI class.
///
/// Be very careful where you hold a shared_ptr to your plugin class from,
/// as it could prevent your plugin class from getting destroyed properly.
///////////////////////////////////////////////////////////////////////////////
FB::JSAPIPtr CVMWeb::createJSAPI()
{

    // m_host is the BrowserHost
    return boost::make_shared<CVMWebAPI>(FB::ptr_cast<CVMWeb>(shared_from_this()), m_host);
}

bool CVMWeb::onMouseDown(FB::MouseDownEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse down at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}

bool CVMWeb::onMouseUp(FB::MouseUpEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse up at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}

bool CVMWeb::onMouseMove(FB::MouseMoveEvent *evt, FB::PluginWindow *)
{
    //printf("Mouse move at: %d, %d\n", evt->m_x, evt->m_y);
    return false;
}
bool CVMWeb::onWindowAttached(FB::AttachedEvent *evt, FB::PluginWindow *)
{
    // The window is attached; act appropriately
    return false;
}

bool CVMWeb::onWindowDetached(FB::DetachedEvent *evt, FB::PluginWindow *)
{
    // The window is about to be detached; act appropriately
    return false;
}

/**
 * Return the path of the DLL
 */
std::string CVMWeb::getFilesystemPath() {
    // Return the path where the DLL is
    return m_filesystemPath;
}

/**
 * Remove a trailing folder from the given path
 */
std::string stripComponent( std::string path ) {
    /* Find the last trailing slash */
    size_t iPos = path.find_last_of('/');
    if (iPos == std::string::npos) { // Check for windows-like trailing path
        iPos = path.find_last_of('\\');
    }
    
    /* Keep only path */
    return path.substr(0, iPos);
}

/**
 * Get the root folder where the plugin data is located.
 * On the XPI build that's the root folder of the extracted XPI contents
 */
std::string CVMWeb::getDataFolderPath() {

    /* Practically dirname() */
    std::string dPath = stripComponent( m_filesystemPath );
    
    /* On OSX we are inside the .plugin/Resources/MacOS/ bundle */
    #if defined(__APPLE__) && defined(__MACH__)
        dPath = stripComponent( dPath );
        dPath = stripComponent( dPath );
        dPath = stripComponent( dPath );
    #endif
    
    /* One folder parent to the DLL */
    dPath = stripComponent( dPath );
    
    /* Return path component */
    return dPath;
}

