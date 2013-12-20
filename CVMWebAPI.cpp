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

  Auto-generated CVMWebAPI.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "DOM/Window.h"
#include "global/config.h"
#include "URI.h"

#include "CVMWebAPI.h"
#include "CVMCallbacks.h"
#include "CVMWebAPISession.h"
#include "CVMWebAPIDaemon.h"
#include "CVMWebLocalConfig.h"
#include "CVMWebInteraction.h"

#include "CVMDialogs.h"

#include "json/json.h"
#include "fbjson.h"

using namespace std;

/** =========================================== **\
                   Tool Functions
\** =========================================== **/

/**
 * Calculate the domain ID using user's unique ID plus the domain name
 * specified.
 */
std::string CVMWebAPI::calculateHostID( std::string& domain ) {
    CRASH_REPORT_BEGIN;

    /* Get a plugin reference */
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) return "";
    
    /* Fetch/Generate user UUID */
    string machineID = this->config->get("local-id");
    if (machineID.empty()) {
        machineID = p->crypto->generateSalt();
        this->config->set("local-id", machineID);
    }

    /* When we use the local-id, update the crash-reporting utility config */
    crashReportAddInfo("Machine UUID", machineID);
    
    /* Create a checksum of the user ID + domain and use this as HostID */
    string checksum = "";
    sha256_buffer( machineID + "|" + domain, &checksum );
    return checksum;

    CRASH_REPORT_END;
}

/**
 * Import the contents of an FB::VariantMap to a ParameterMap
 */
void importVariantMap( ParameterMapPtr pm, FB::VariantMap& vm ) {
    CRASH_REPORT_BEGIN;
    pm->lock();
    for (FB::VariantMap::iterator it = vm.begin(); it != vm.end(); ++it) {
        std::string k = (*it).first;
        FB::variant v = (*it).second;
        try {
            pm->set( k, v.convert_cast<std::string>() );
        } catch (FB::bad_variant_cast e&) {
        }
    }
    pm->unlock();
    CRASH_REPORT_END;
}


/** =========================================== **\
                 Plugin Functions
\** =========================================== **/

///////////////////////////////////////////////////////////////////////////////
/// @fn CVMWebPtr CVMWebAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
CVMWebPtr CVMWebAPI::getPlugin()
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
 * Check if a hypervisor was detected
 */
bool CVMWebAPI::hasHypervisor() {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) return false;
    return (p->hv->getType() != HV_NONE);
    CRASH_REPORT_END;
};

// Read-only property version
std::string CVMWebAPI::get_version() {
    CRASH_REPORT_BEGIN;
    return FBSTRING_PLUGIN_VERSION;
    CRASH_REPORT_END;
}

std::string CVMWebAPI::get_hv_name() {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) {
        return "";
    } else {
        if (p->hv->getType() == HV_VIRTUALBOX) {
            return "virtualbox";
        } else {
            return "unknown";
        }
    }
    CRASH_REPORT_END;
}

std::string CVMWebAPI::get_hv_version() {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) {
        return "";
    } else {
        return p->hv->version.verString;
    }
    CRASH_REPORT_END;
}

/**
 * Define the callback functions for user interaction
 */
void CVMWebAPI::setUserInteraction( const FB::variant& v ) {
    CRASH_REPORT_BEGIN;

    // Replace the user interaction with the one specified
    // by javascript
    if (IS_CB_AVAILABLE(v)) {
        userInteraction = CVMWebInteraction::fromJSObject( v );
    }

    CRASH_REPORT_END;
}

/**
 * Return the current domain name
 */
std::string CVMWebAPI::getDomainName() {
    CRASH_REPORT_BEGIN;
    FB::URI loc = FB::URI::fromString(m_host->getDOMWindow()->getLocation());
    return loc.domain;
    CRASH_REPORT_END;
};

/**
 * Check if the current domain is priviledged
 */
bool CVMWebAPI::isDomainPrivileged() {
    CRASH_REPORT_BEGIN;
    std::string domain = this->getDomainName();
    
    // Domain is empty only when we see the plugin from a file:// URL
    // (And yes, even localhost is not considered priviledged)
    return domain.empty();
    CRASH_REPORT_END;
}

/**
 * Check the status of the given session
 */
FB::variant CVMWebAPI::checkSession( const FB::variant& vm, const FB::variant& secret ) {
    CRASH_REPORT_BEGIN;
    
    // Check for invalid plugin
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) return CVME_UNSUPPORTED;
    
    // Try to open the session
    string vmName = vm.convert_cast<string>();
    string vmSecret = secret.convert_cast<string>();

    // Prepare parameters for session validation
    ParameterMapPtr parm = ParameterMap::instance();
    parm->set("name", vmName);
    parm->set("key", vmSecret);

    // Validate session and return response
    int ans = p->hv->sessionValidate( parm );
    return ans;
    
    CRASH_REPORT_END;
}

/**
 * Request a new session using the safe URL
 */
FB::variant CVMWebAPI::requestSafeSession( const FB::variant& vmcpURL, const FB::variant &callbacks ) {
    CRASH_REPORT_BEGIN;

    // Create the object where we can forward the events
    JSObjectCallbacks cb( callbacks );
    
    // Block requests when reached throttled state
    if (this->throttleBlock) {
        cb.fire("failed", ArgumentList( "Request denied by throttle protection" )( CVME_ACCESS_DENIED ) );
        return CVME_ACCESS_DENIED;
    }

    // Check for invalid plugin
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) {
        cb.fire("failed", ArgumentList( "A hypervisor was not detected" )( CVME_UNSUPPORTED ) );
        return CVME_UNSUPPORTED;
    }

    // Schedule thread exec
    lastThread = boost::thread(boost::bind(&CVMWebAPI::requestSafeSession_thread, this, vmcpURL, callbacks));

    // Scheduled for creation
    return HVE_SCHEDULED;
        
    CRASH_REPORT_END;
}

/**
 * The thread of requesting new session
 */
void CVMWebAPI::requestSafeSession_thread( const FB::variant& vmcpURL, const FB::variant &callbacks ) {
    CRASH_REPORT_BEGIN;

    try {

        // Create the object where we can forward the events
        JSObjectCallbacks cb( callbacks );

        // Create a progress feedback mechanism
        FiniteTaskPtr pTasks = boost::make_shared<FiniteTask>();
        pTasks->setMax( 2 );
        cb.listen( pTasks );

        // Create two sub-tasks that will be used for equally
        // dividing the progress into two tasks: validate and start
        FiniteTaskPtr pInit = pTasks->begin<VariableTask>( "Preparing for session request" );
        pInit->setMax( 4 );

        // =======================================================================

        // Wait for delaied hypervisor initiation
        p->hv->waitTillReady( pInit->begin<VariableTask>( "Initializing hypervisor" ) );

        // =======================================================================

        // Fetch domain info
        std::string domain = this->getDomainName();

        // Try to update authorized keystore if it's in an invalid state
        pInit->doing("Initializing crypto store");
        if (!isDomainPrivileged()) {
        
            // Trigger update in the keystore (if it's nessecary)
            p->crypto->updateAuthorizedKeystore( p->browserDownloadProvider );

            // Still invalid? Something's wrong
            if (!p->crypto->valid) {
                cb.fire("failed", ArgumentList( "Unable to initialize cryptographic store" )( CVME_NOT_VALIDATED ) );
                return;
            }

            // Block requests from untrusted domains
            if (!p->crypto->isDomainValid(domain)) {
                cb.fire("failed", ArgumentList( "The domain is not trusted" )( CVME_NOT_TRUSTED ) );
                return;
            }
        
        }
        pInit->done("Crypto store initialized");
    
        // =======================================================================

        // Validate arguments
        pInit->doing("Contacting the VMCP endpoint");
        std::string sURL = vmcpURL.convert_cast<std::string>();
        if (sURL.empty()) {
            cb.fire("failed", ArgumentList( "Missing VMCP URL from the request" )( HVE_USAGE_ERROR ) );
            return;
        }
    
        // Put salt and user-specific ID in the URL
        std::string salt = p->crypto->generateSalt();
        std::string glueChar = "&";
        if (sURL.find("?") == string::npos) glueChar = "?";
        std::string newURL = 
            sURL + glueChar + 
            "cvm_salt=" + salt + "&" +
            "cvm_hostid=" + this->calculateHostID( domain );
    
        // Download data from URL
        std::string jsonString;
        int res = p->browserDownloadProvider->downloadText( newURL, &jsonString );
        if (res < 0) {
            cb.fire("failed", ArgumentList( "Unable to contact the VMCP endpoint" )( res ) );
            return;
        }

        pInit->doing("Validating VMCP data");

        // Try to parse the data
        FB::variant jsonData = FB::jsonToVariantValue( jsonString );
        if (!jsonData.is_of_type<FB::VariantMap>()) {
            cb.fire("failed", ArgumentList( "Unable to parse response data as JSON" )( HVE_QUERY_ERROR ) );
            return;
        }
    
        // Import response to a ParameterMap
        FB::VariantMap jsonHash = jsonData.cast<FB::VariantMap>();
        ParameterMapPtr vmcpData = ParameterMap::instance();
        importVariantMap( vmcpData, jsonHash );

        // Validate response
        if (!vmcpData->contains("name")) {
            cb.fire("failed", ArgumentList( "Missing 'name' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (!vmcpData->contains("secret")) {
            cb.fire("failed", ArgumentList( "Missing 'secret' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (!vmcpData->contains("signature")) {
            cb.fire("failed", ArgumentList( "Missing 'signature' parameter from the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        };
        if (vmcpData->contains("diskURL") && !vmcpData->contains("diskChecksum")) {
            cb.fire("failed", ArgumentList( "A 'diskURL' was specified, but no 'diskChecksum' was found in the VMCP response" )( HVE_USAGE_ERROR ) );
            return;
        }

        // Validate signature
        res = p->crypto->signatureValidate( domain, salt, jsonHash );
        if (res < 0) {
            cb.fire("failed", ArgumentList( "The VMCP response signature could not be validated" )( res ) );
            return;
        }

        pInit->done("Obtained information from VMCP endpoint");

        // =======================================================================
    
        // Check session state
        res = p->hv->sessionValidate( vmcpData );
        if (res == 2) { 
            // Invalid password
            cb.fire("failed", ArgumentList( "The password specified is invalid for this session" )( CVME_PASSWORD_DENIED ) );
            return;
        }

        // =======================================================================
    
        /* Check if the session is new and prompt the user */
        pInit->doing("Validating request");
        if (res == 0) {
            pInit->doing("Session is new, asking user for confirmation");

            // Newline-specific split
            string msg = "The website " + domain + " is trying to allocate a " + this->get_hv_name() + " Virtual Machine \"" + vmcpData->get("name") + "\". This website is validated and trusted by CernVM." _EOL _EOL "Do you want to continue?";

            // Prompt user using the currently active userInteraction 
            if (userInteraction->confirm("New CernVM WebAPI Session", msg) != UI_OK) {
            
                // If we were aborted due to shutdown, exit
                if (this->shuttingDown) return;

                // Manage throttling 
                if ((getMillis() - this->throttleTimestamp) <= THROTTLE_TIMESPAN) {
                    if (++this->throttleDenies >= THROTTLE_TRIES)
                        this->throttleBlock = true;
                } else {
                    this->throttleDenies = 1;
                    this->throttleTimestamp = getMillis();
                }

                // Fire error
                cb.fire("failed", ArgumentList( "User denied the allocation of new session" )( CVME_ACCESS_DENIED ) );
                return;
            
            } else {
            
                // Reset throttle
                this->throttleDenies = 0;
                this->throttleTimestamp = 0;
            
            }
        
        }
        pInit->done("Request validated");

        // =======================================================================

        // Prepare a progress task that will be used by sessionOpen    
        FiniteTaskPtr pOpen = pTasks->begin<VariableTask>( "Oppening session" );

        // Open/resume session
        HVSessionPtr session = p->hv->sessionOpen( vmcpData, userInteraction, pOpen );
        if (!session) {
            cb.fire("failed", ArgumentList( "Unable to open session" )( CVME_ACCESS_DENIED ) );
            return;
        }

        // We have everything. Prepare CVMWebAPI Session and fire success
        boost::shared_ptr<CVMWebAPISession> pSession = boost::make_shared<CVMWebAPISession>(p, m_host, session );
        pTasks->complete( "Session oppened successfully" );
        cb.fire("succeed", ArgumentList( "Session oppened successfully" ) );
        
        // Check if we need a daemon for our current services
        p->hv->checkDaemonNeed();
    
    } catch (...) {

        CVMWA_LOG("Error", "Exception occured!");

        // Raise failure
        cb.fire("failed", ArgumentList( "Unexpected exception occured while requesting session" )( HVE_EXTERNAL_ERROR ) );

    }

    CRASH_REPORT_END;
}


/**
 * Hypervisor installation thread
 */
void CVMWebAPI::thread_install( const FB::variant& callbacks ) {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();

    // Create the object where we can forward the events
    JSObjectCallbacks cb( callbacks );

    // Create a progress feedback mechanism
    FiniteTaskPtr pTasks = boost::make_shared<FiniteTask>();
    cb.listen( pTasks );

    // Install hypervisor
    int ans = installHypervisor( p->browserDownloadProvider, userInteraction, pTasks );
    if (ans == HVE_OK) {

        /* Update our hypervisor pointer */
        p->hv = detectHypervisor();
        if (p->hv) {
            p->hv->daemonBinPath = p->getDaemonBin();
            p->hv->setDownloadProvider( p->browserDownloadProvider );
            p->hv->loadSessions();
        }

        this->fire_install();
    } else {
        this->fire_installError(hypervisorErrorStr(ans), ans);
    }
    CRASH_REPORT_END;
};

/**
 * Request a hypervisor installation
 */
int CVMWebAPI::installHypervisor( const FB::variant& callbacks ) {
    CRASH_REPORT_BEGIN;

    // Create the object where we can forward the events
    JSObjectCallbacks cb( callbacks );

    // If we already have a hypervisor, we don't need to install it
    if ( hasHypervisor() ) {
        cb.fire("failed", ArgumentList( "A hypervisor is already installed in this system" )( HVE_INVALID_STATE ) );
        return HVE_INVALID_STATE;
    }
    
    /* Start installation thread */
    boost::thread t(boost::bind(&CVMWebAPI::thread_install, this, callbacks ));
    return HVE_SCHEDULED;
    
    CRASH_REPORT_END;
};

/**
 * Javascript confirmation callback for unsafeConfirm + CVM library
 */
void CVMWebAPI::confirmCallback( const FB::variant& status ) {
    CRASH_REPORT_BEGIN;
    if (!status.is_of_type<bool>()) return;

    // Fetch mapping info
    bool result = status.convert_cast<bool>();

    // Lookup callback info
    {
        boost::lock_guard<boost::mutex> lock(confirmMutex);
        confirmResult = result;
    }
    confirmCond.notify_one();

    CRASH_REPORT_END;
};

/**
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::unsafeConfirm( std::string msg ) {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    FB::variant f;
    
    // Retrieve a reference to the DOM Window
    FB::DOM::WindowPtr window = m_host->getDOMWindow();

    // Check if the DOM Window has an alert property
    if (window && window->getJSObject()->HasProperty("window")) {

        // Get window reference
        FB::JSObjectPtr jsWin = window->getProperty<FB::JSObjectPtr>("window");
        
        // Check if we have CVM library in place
        if (jsWin->HasProperty("CVM")) {

            // Get a reference to CVM
            f = jsWin->GetProperty("CVM");
            FB::JSObjectPtr jsCVM = f.convert_cast<FB::JSObjectPtr>();
            
            // Only use global confirm function if CVM provides it
            if (jsCVM->HasProperty("__globalConfirm")) {
            
                // Fire callback
                jsCVM->InvokeAsync("__globalConfirm", FB::variant_list_of(msg));

                // Cancel previous confirm
                if (pendingConfirm) {
                    {
                        boost::lock_guard<boost::mutex> lock(confirmMutex);
                        confirmResult = false;
                    }
                    confirmCond.notify_one();
                }

                // Lock while waiting for response
                pendingConfirm = true;
                confirmResult = false;
                boost::unique_lock<boost::mutex> lock(confirmMutex);
                confirmCond.wait(lock);
                pendingConfirm = false;

                // Return status
                return confirmResult;

            }
        }

        /*
        // Get a reference to the confirm function
        f = jsWin->GetProperty("confirm");
        FB::JSObjectPtr jsConfirm = f.convert_cast<FB::JSObjectPtr>();

        // Get a reference to Function object
        f = jsWin->GetProperty("Function");
        FB::JSObjectPtr jsFunction = f.convert_cast<FB::JSObjectPtr>();

        // Get a reference fo Function.prototype object
        f = jsFunction->GetProperty("prototype");
        FB::JSObjectPtr jsFunctionProto = f.convert_cast<FB::JSObjectPtr>();

        // Get a reference to Object object
        f = jsWin->GetProperty("Object");
        FB::JSObjectPtr jsObject = f.convert_cast<FB::JSObjectPtr>();

        // 1) First make sure alert's toString is not directly hijacked. However:
        //    - Function.prototype.toString might be hijacked
        if (jsObject->HasProperty("toString")) {
            return false;
        }

        // Now make sure (using toString) that is native code
        string fType = jsConfirm->Invoke("toString", FB::variant_list_of( msg )).convert_cast<string>();
        CVMWA_LOG("Debug", "Function is '" << fType << "'");
        if (fType.find("[native code]") == string::npos)
            return false;
        */

        // Invoke confirm with some text
        return jsWin->Invoke("confirm", FB::variant_list_of( msg )).convert_cast<bool>();

    } else {
        return false;
    }
    
    CRASH_REPORT_END;
}

/**
 * Return the last Error that occured to the plug-in
 */
std::string CVMWebAPI::get_lastError() {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) return "No hypervisor available";

    /* Get and reset last error */
    string lastError = p->hv->lastExecError;
    p->hv->lastExecError = "";
    return lastError;
    CRASH_REPORT_END;
}

/**
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::confirm( std::string msg ) {
    CRASH_REPORT_BEGIN;
    
    #ifdef BROWSER_CONFIRM
    
        return unsafeConfirm( msg );

    #else
    
        CVMWebPtr p = this->getPlugin();
        return CVMConfirmDialog( m_host, p->GetWindow(), msg );

    #endif
    
    CRASH_REPORT_END;
}

/**
 * Get a access to the idle daemon
 */
FB::variant CVMWebAPI::requestDaemonAccess( const FB::variant &successCb, const FB::variant &failureCb ) {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return CVME_UNSUPPORTED;
    } else {
        if (IS_CB_AVAILABLE(successCb)) successCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of(
            boost::make_shared<CVMWebAPIDaemon>(p, m_host)
        ));
        return HVE_SCHEDULED;
    }
    CRASH_REPORT_END;
}

/**
 * Get a access to the control daemon
 */
FB::variant CVMWebAPI::requestControlAccess( const FB::variant &successCb, const FB::variant &failureCb ) {
    CRASH_REPORT_BEGIN;
    CVMWebPtr p = this->getPlugin();
    if (!p->hv) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return CVME_UNSUPPORTED;
    } else {
        if (!isDomainPrivileged()) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
            return CVME_ACCESS_DENIED;
        } else {
            if (IS_CB_AVAILABLE(successCb)) successCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of(
                boost::make_shared<CVMWebLocalConfig>(p, m_host)
            ));
            return HVE_SCHEDULED;
        }
    }
    CRASH_REPORT_END;
}

std::string CVMWebAPI::toString() {
    CRASH_REPORT_BEGIN;
    return "[CVMWebAPI]";
    CRASH_REPORT_END;
}