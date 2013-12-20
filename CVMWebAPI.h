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

  Auto-generated CVMWebAPI.h

\**********************************************************/

#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "BrowserHost.h"

#include "Common/Utilities.h"
#include "Common/LocalConfig.h"
#include "Common/CrashReport.h"

#include "CVMGlobals.h"
#include "CVMWeb.h"
#include "CVMBrowserProvider.h"

#ifndef H_CVMWebAPI
#define H_CVMWebAPI

#define CVME_OK                 0
#define CVME_ACCESS_DENIED      -10 /* Same to HVE_NOT_ALLOWED */
#define CVME_UNSUPPORTED        -11 /* Same to HVE_NOT_SUPPORTED */
#define CVME_NOT_VALIDATED      -12 /* Same to HVE_NOT_VALIDATED */
#define CVME_NOT_TRUSTED        -13 /* Same to HVE_NOT_TRUSTED */
#define CVME_PASSWORD_DENIED    -20

class CVMWebAPI : public FB::JSAPIAuto
{
public:
    ////////////////////////////////////////////////////////////////////////////
    /// @fn CVMWebAPI::CVMWebAPI(const CVMWebPtr& plugin, const FB::BrowserHostPtr host)
    ///
    /// @brief  Constructor for your JSAPI object.
    ///         You should register your methods, properties, and events
    ///         that should be accessible to Javascript from here.
    ///
    /// @see FB::JSAPIAuto::registerMethod
    /// @see FB::JSAPIAuto::registerProperty
    /// @see FB::JSAPIAuto::registerEvent
    ////////////////////////////////////////////////////////////////////////////
    CVMWebAPI(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host) :
        m_plugin(plugin), m_host(host)
    {

        registerMethod("checkSession",        make_method(this, &CVMWebAPI::checkSession));
        registerMethod("requestDaemonAccess", make_method(this, &CVMWebAPI::requestDaemonAccess));
        registerMethod("requestControlAccess",make_method(this, &CVMWebAPI::requestControlAccess));
        registerMethod("requestSafeSession",  make_method(this, &CVMWebAPI::requestSafeSession));
        
        registerMethod("installHypervisor",   make_method(this, &CVMWebAPI::installHypervisor));
        registerMethod("setUserInteraction",  make_method(this, &CVMWebAPI::setUserInteraction));

        // Read-only property
        registerProperty("version",           make_property(this, &CVMWebAPI::get_version));
        registerProperty("hypervisorName",    make_property(this, &CVMWebAPI::get_hv_name));
        registerProperty("hypervisorVersion", make_property(this, &CVMWebAPI::get_hv_version));
        registerProperty("domain",            make_property(this, &CVMWebAPI::getDomainName));
        registerProperty("lastError",         make_property(this, &CVMWebAPI::get_lastError));
        
        // Beautification
        registerMethod("toString",          make_method(this, &CVMWebAPI::toString));
        registerMethod("confirmCallback",   make_method(this, &CVMWebAPI::confirmCallback));

        // Reset AuthType
        this->m_authType = 0;
        this->throttleTimestamp = 0;
        this->throttleDenies = 0;
        this->throttleBlock = false;
        this->shuttingDown = false;

        // Get global config pointer
        config = LocalConfig::global();

        // At construction, use the default user interaction class
        userInteraction = UserInteraction::Default();
        
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// @fn CVMWebAPI::~CVMWebAPI()
    ///
    /// @brief  Destructor.  Remember that this object will not be released until
    ///         the browser is done with it; this will almost definitely be after
    ///         the plugin is released.
    ///////////////////////////////////////////////////////////////////////////////
    virtual ~CVMWebAPI() {
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Called when the plugin is about to shutdown -> Abort any confirmation process
    ///////////////////////////////////////////////////////////////////////////////
    virtual void shutdown() {
        this->shuttingDown = true;
        
        // If we are pending confirm, cleanup thread
        if (pendingConfirm) {

            // Release condition variable
            {
                boost::lock_guard<boost::mutex> lock(confirmMutex);
                confirmResult = false;
            }
            confirmCond.notify_one();

            // Wait thead to complete
            this->lastThread.join();

        }
        
        // Flush named mutexes stack
        // (We are free to do so at any time)
        flushNamedMutexes( );

    }

    CVMWebPtr getPlugin();

    // Read-only property ${PROPERTY.ident}
    std::string get_version();
    std::string get_hv_name();
    std::string get_hv_version();
    std::string get_lastError();

    // Threads
    void thread_install( );

    // Request safe session
    FB::variant requestSafeSession( const FB::variant& vmcpURL, const FB::variant &callbacks );
    void requestSafeSession_thread( const FB::variant& vmcpURL, const FB::variant &callbacks );

    // JS Callbacks
    void confirmCallback( const FB::variant& status );

    // Methods
    FB::variant checkSession( const FB::variant& vm, const FB::variant& code );
    FB::variant requestDaemonAccess( const FB::variant &successCb, const FB::variant &failureCb );
    FB::variant requestControlAccess( const FB::variant &successCb, const FB::variant &failureCb );
    std::string getDomainName();
    std::string toString();
    void        setUserInteraction( const FB::variant& v );
    int         installHypervisor( const FB::variant& callbacks );
    bool        hasHypervisor();
    
    // Events
    FB_JSAPI_EVENT(install,         0, ());
    FB_JSAPI_EVENT(installError,    2, ( const std::string&, int ));
    FB_JSAPI_EVENT(installProgress, 3, ( const size_t, const size_t, const std::string& ));
    
    // Forward proxy to browser's confirm
    bool        confirm( std::string );
    bool        unsafeConfirm( std::string msg );
    
    // Common configuration class
    LocalConfigPtr      config;
    
    // Event delegates
    void                onInstallProgress( const size_t step, const size_t total, const std::string& msg );

    // Local user interaction instance
    UserInteractionPtr  userInteraction;

private:
    
    CVMWebWeakPtr       m_plugin;
    FB::BrowserHostPtr  m_host;
    int                 m_authType;

    bool                isDomainPrivileged();

    // Throttling protection
    long                throttleTimestamp;
    int                 throttleDenies;
    bool                throttleBlock;
    
    // Host ID calculation
    std::string         calculateHostID( std::string& domain );
    
    // Synchronization
    boost::mutex                confirmMutex;
    boost::condition_variable   confirmCond;
    bool                        confirmResult;
    bool                        pendingConfirm;
    bool                        shuttingDown;
    boost::thread               lastThread;

};

#endif // H_CVMWebAPI
