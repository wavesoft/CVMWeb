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
#include "CVMWebAPISession.h"
#include "CVMWebAPIDaemon.h"
#include "CVMWebLocalConfig.h"

#include "Dialogs.h"

#include "json/json.h"
#include "fbjson.h"

using namespace std;

/** =========================================== **\
                   Tool Functions
\** =========================================== **/

/**
 * Delegate function to forward progress events to a javascript object
 */
class ProgressDelegate {
public:

    FB::JSObjectPtr         jsFunction;
    bool                    available;

    /**
     * Initialize delegate class
     */
    ProgressDelegate( const FB::variant& o ) {
        available = false;
        if (IS_CB_AVAILABLE(o)) {
            available = true;
            jsFunction = o.cast<FB::JSObjectPtr>();
        }
    }

    /**
     * Delegate function
     */
    void fire(const size_t v, const size_t tot, const std::string& msg) {
        if (available) 
            jsFunction->InvokeAsync("", FB::variant_list_of(v)(tot)(msg));
    }

    /**
     * Get access to the function
     */
    callbackProgress getFunction() {
        return boost::bind( &ProgressDelegate::fire, this, _1, _2, _3 );
    }

};

/**
 * Forward progress events
 */
void CVMWebAPI::onInstallProgress( const size_t step, const size_t total, const std::string& msg ) {
    this->fire_installProgress( step, total, msg );
}

/**
 * Calculate the domain ID using user's unique ID plus the domain name
 * specified.
 */
std::string CVMWebAPI::calculateHostID( std::string& domain ) {

    /* Get a plugin reference */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) return "";
    
    /* Fetch/Generate user UUID */
    string machineID = this->config.get("local-id");
    if (machineID.empty()) {
        machineID = p->crypto->generateSalt();
        this->config.set("local-id", machineID);
    }
    
    /* Create a checksum of the user ID + domain and use this as HostID */
    string checksum = "";
    sha256_buffer( machineID + "|" + domain, &checksum );
    return checksum;
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
    CVMWebPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}

/**
 * Check if a hypervisor was detected
 */
bool CVMWebAPI::hasHypervisor() {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) return false;
    return (p->hv->type != 0);
};

// Read-only property version
std::string CVMWebAPI::get_version() {
    return FBSTRING_PLUGIN_VERSION;
}

std::string CVMWebAPI::get_hv_name() {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        return "";
    } else {
        if (p->hv->type == 1) {
            return "virtualbox";
        } else if (p->hv->type == 2) {
            return "vmware-player";
        } else {
            return "unknown";
        }
    }
}

std::string CVMWebAPI::get_hv_version() {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        return "";
    } else {
        return p->hv->verString;
    }
}

/**
 * Grant access to the plugin API
 */
int CVMWebAPI::authenticate( const std::string& key ) {
    this->m_authType = 1;
    return HVE_OK;
};

/**
 * Return the current domain name
 */
std::string CVMWebAPI::getDomainName() {
    FB::URI loc = FB::URI::fromString(m_host->getDOMWindow()->getLocation());
    return loc.domain;
};

/**
 * Check if the current domain is priviledged
 */
bool CVMWebAPI::isDomainPrivileged() {
    std::string domain = this->getDomainName();
    
    /* Domain is empty only when we see the plugin from a file:// URL
     * (And yes, even localhost is not considered priviledged) */
    return domain.empty();
}

/**
 * Check the status of the given session
 */
FB::variant CVMWebAPI::checkSession( const FB::variant& vm, const FB::variant& secret ) {
    
    /* Check for invalid plugin */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) return CVME_UNSUPPORTED;
    
    /* Try to open the session */
    string vmName = vm.convert_cast<string>();
    string vmSecret = secret.convert_cast<string>();
    int ans = p->hv->sessionValidate( vmName, vmSecret );
    return ans;
    
}

/**
 * Request a new session using the safe URL
 */
FB::variant CVMWebAPI::requestSafeSession( const FB::variant& vmcpURL, const FB::variant &successCb, const FB::variant &failureCb, const FB::variant &progressCB ) {
    
    /* Block requests when reached throttled state */
    if (this->throttleBlock) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
        return CVME_ACCESS_DENIED;
    }

    /* Check for invalid plugin */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) return CVME_UNSUPPORTED;
    
    /* Schedule thread exec */
    lastThread = boost::thread(boost::bind(&CVMWebAPI::requestSafeSession_thread,
         this, vmcpURL, successCb, failureCb, progressCB));

    /* Scheduled for creation */
    return HVE_SCHEDULED;
        
}

/**
 * The thread of requesting new session
 */
void CVMWebAPI::requestSafeSession_thread( const FB::variant& vmcpURL, const FB::variant &successCb, const FB::variant &failureCb, const FB::variant &progressCB ) {

    /* Handle request */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return;
    }

    /* Create a progress delegate */
    ProgressDelegate onProgress( progressCB );
    onProgress.fire( 1, 7, "Initializing hypervisor" );

    /* Wait for delaied hypervisor initiation */
    p->hv->waitTillReady();

    /* Fetch domain info */
    std::string domain = this->getDomainName();

    /* Try to update authorized keystore if it's in an invalid state */
    onProgress.fire( 2, 7, "Initializing crypto store" );
    if (!isDomainPrivileged()) {
        
        // Trigger update in the keystore (if it's nessecary)
        p->crypto->updateAuthorizedKeystore( p->browserDownloadProvider );

        // Still invalid? Something's wrong
        if (!p->crypto->valid) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_NOT_VALIDATED ));
            return;
        }

        // Block requests from untrusted domains
        if (!p->crypto->isDomainValid(domain)) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_NOT_TRUSTED ));
            return;
        }
        
    }
    
    /* Validate arguments */
    onProgress.fire( 3, 7, "Contacting configuration server" );
    std::string sURL = vmcpURL.convert_cast<std::string>();
    if (sURL.empty()) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_USAGE_ERROR ));
        return;
    }
    
    /* Put salt and user-specific ID in the URL */
    std::string salt = p->crypto->generateSalt();
    std::string glueChar = "&";
    if (sURL.find("?") == string::npos) glueChar = "?";
    std::string newURL = 
        sURL + glueChar + 
        "cvm_salt=" + salt + "&" +
        "cvm_hostid=" + this->calculateHostID( domain );
    
    /* Download data from URL */
    std::string jsonString;
    int res = p->browserDownloadProvider->downloadText( newURL, &jsonString );
    if (res < 0) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( res ));
        return;
    }
    
    /* Try to parse the data */
    FB::variant jsonData = FB::jsonToVariantValue( jsonString );
    if (!jsonData.is_of_type<FB::VariantMap>()) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_QUERY_ERROR ));
        return;
    }
    
    /* Validate response */
    FB::VariantMap jsonHash = jsonData.cast<FB::VariantMap>();
    if (jsonHash.find("name") == jsonHash.end()) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_USAGE_ERROR ));
        return;
    };
    if (jsonHash.find("secret") == jsonHash.end()) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_USAGE_ERROR ));
        return;
    };
    if (jsonHash.find("signature") == jsonHash.end()) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_USAGE_ERROR ));
        return;
    };
    
    /* Validate signature */
    onProgress.fire( 4, 7, "Validating server identity" );
    res = p->crypto->signatureValidate( domain, salt, jsonHash );
    if (res < 0) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( res ));
        return;
    }
    
    /* Fetch useful fields */
    string vmName = jsonHash["name"].convert_cast<string>();
    string vmSecret = jsonHash["secret"].convert_cast<string>();
    
    /* Check session state */
    res = p->hv->sessionValidate( vmName, vmSecret );
    if (res == 2) { 
        // Invalid password
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
        return;
    }
    
    /* Check if the session is new and prompt the user */
    if (res == 0) {
        onProgress.fire( 5, 7, "Starting new session" );

        // Newline-specific split
        string msg = "The website '" + domain + "' is trying to allocate a " + this->get_hv_name() + " Virtual Machine. This website is validated and trusted by CernVM." _EOL _EOL "Do you want to continue?";

        // Prompt user
        // (It is safe to use unsafe confirm, since we trust the domain - Therefore we also get rid of all the platform-dependant code)
        if (!this->unsafeConfirm(msg)) {
            
            /* If we were aborted due to shutdown, exit */
            if (this->shuttingDown) return;

            /* Manage throttling */
            if ((getMillis() - this->throttleTimestamp) <= THROTTLE_TIMESPAN) {
                if (++this->throttleDenies >= THROTTLE_TRIES)
                    this->throttleBlock = true;
            } else {
                this->throttleDenies = 1;
                this->throttleTimestamp = getMillis();
            }
            
            onProgress.fire( 7, 7, "User aborted" );
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
            return;
            
        } else {
            
            /* Reset throttle */
            this->throttleDenies = 0;
            this->throttleTimestamp = 0;
            
        }
        
    }
    
    /* Open/resume session */
    onProgress.fire( 6, 7, "Oppening session" );
    HVSession * session = p->hv->sessionOpen(vmName, vmSecret);
    if (session == NULL) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
        return;
    }
    
    /* Update defaults on the session */
    if (jsonHash.find("ram") != jsonHash.end())              session->memory = jsonHash["ram"].convert_cast<int>();
    if (jsonHash.find("cpus") != jsonHash.end())             session->cpus = jsonHash["cpus"].convert_cast<int>();
    if (jsonHash.find("disk") != jsonHash.end())             session->disk = jsonHash["disk"].convert_cast<int>();
    if (jsonHash.find("flags") != jsonHash.end())            session->flags = jsonHash["flags"].convert_cast<int>();
    if (jsonHash.find("version") != jsonHash.end())          session->version = jsonHash["version"].convert_cast<string>();
    if (jsonHash.find("apiPort") != jsonHash.end())          session->apiPort = jsonHash["apiPort"].convert_cast<int>();
    if (jsonHash.find("userData") != jsonHash.end())         session->userData = jsonHash["userData"].convert_cast<string>();
    if (jsonHash.find("executionCap") != jsonHash.end())     session->executionCap = jsonHash["executionCap"].convert_cast<int>();
    if (jsonHash.find("daemonControlled") != jsonHash.end()) session->daemonControlled = jsonHash["daemonControlled"].convert_cast<bool>();
    if (jsonHash.find("daemonMinCap") != jsonHash.end())     session->daemonMinCap = jsonHash["daemonMinCap"].convert_cast<int>();
    if (jsonHash.find("daemonMaxCap") != jsonHash.end())     session->daemonControlled = jsonHash["daemonMaxCap"].convert_cast<int>();
    if (jsonHash.find("daemonFlags") != jsonHash.end())      session->daemonFlags = jsonHash["daemonFlags"].convert_cast<int>();
    if (jsonHash.find("diskURL") != jsonHash.end()) {
        
        // If we have a missing checksum, that's a problem
        if (jsonHash.find("diskChecksum") == jsonHash.end()) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( HVE_USAGE_ERROR ));
            return;
        }
        
        // Update information
        session->version = jsonHash["diskURL"].convert_cast<string>();
        session->diskChecksum = jsonHash["diskChecksum"].convert_cast<string>();
        session->flags |= HVF_DEPLOYMENT_HDD;
        
    } else if (jsonHash.find("version") != jsonHash.end()) {
        
        // Update the default flags for CernVM Micro
        session->flags |= HVF_SYSTEM_64BIT;
        
    }
    
    /* Get the overridable vars */
    session->overridableVars.clear();
    if (jsonHash.find("canOverride") != jsonHash.end())
        explode(jsonHash["canOverride"].convert_cast<string>(), ',', &session->overridableVars );
    
    CVMWA_LOG("Debug", "userData=" << session->userData);
    CVMWA_LOG("Debug", "cpus=" << session->cpus);
    CVMWA_LOG("Debug", "executionCap=" << session->executionCap);
    CVMWA_LOG("Debug", "apiPort=" << session->apiPort);
    CVMWA_LOG("Debug", "ram=" << session->memory);
    CVMWA_LOG("Debug", "disk=" << session->disk);
    CVMWA_LOG("Debug", "flags=" << session->flags);
    CVMWA_LOG("Debug", "version/diskURL=" << session->version);
    CVMWA_LOG("Debug", "daemonControlled=" << session->daemonControlled);
    CVMWA_LOG("Debug", "daemonMinCap=" << session->daemonMinCap);
    CVMWA_LOG("Debug", "daemonMaxCap=" << session->daemonMaxCap);
    CVMWA_LOG("Debug", "daemonFlags=" << session->daemonFlags);
    CVMWA_LOG("Debug", "diskChecksum=" << session->diskChecksum);
    
    /* Call success callback */
    onProgress.fire( 7, 7, "Session ready" );
    boost::shared_ptr<CVMWebAPISession> pSession = boost::make_shared<CVMWebAPISession>(p, m_host, session );
    if (IS_CB_AVAILABLE(successCb)) successCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( pSession ));
        
    /* Check if we need a daemon for our current services */
    p->hv->checkDaemonNeed();
    
}

/**
 * Create and return a session object. 
 */
FB::variant CVMWebAPI::requestSession( const FB::variant& vm, const FB::variant& secret, const FB::variant &successCb, const FB::variant &failureCb ) {

    /* Fetch domain info */
    std::string domain = this->getDomainName();
    
    /* Block requests when reached throttled state */
    if (this->throttleBlock) return CVME_ACCESS_DENIED;

    /* Check for invalid plugin */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) return CVME_UNSUPPORTED;
    
    /* Schedule thread exec */
    boost::thread t(boost::bind(&CVMWebAPI::requestSession_thread,
         this, vm, secret, successCb, failureCb));

    /* Scheduled for creation */
    return HVE_SCHEDULED;
}

void CVMWebAPI::requestSession_thread( const FB::variant& vm, const FB::variant& secret, const FB::variant &successCb, const FB::variant &failureCb ) {
    
    /* Fetch domain info once again */
    string domain = this->getDomainName();

    /*
    // Try to update authorized keystore if it's in an invalid state
    if (!localDomain) {
        // Trigger update
        if (!this->crypto->valid)
            this->crypto->updateAuthorizedKeystore();

        // Still invalid? Something's wrong
        if (!this->crypto->valid) {
            if (failureCb != NULL) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_NOT_VALIDATED ));
            return;
        }

        // Block requests from untrusted domains
        if (!this->crypto->isDomainValid(domain)) {
            if (failureCb != NULL) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_NOT_TRUSTED ));
            return;
        }
    }
    */
        
    /* Handle request */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return;

    } else {
    
        /* Wait for delaied hypervisor initiation */
        p->hv->waitTillReady();

        /* Fetch info */
        string vmName = vm.convert_cast<string>();
        string vmSecret = secret.convert_cast<string>();
        
        /* Try to open the session */
        int ans = p->hv->sessionValidate( vmName, vmSecret );
        
        /* Notify user that a new session will open */
        if (ans == 0) {

            // Newline-specific split
            string msg = "The website '" + domain + "' is trying to allocate a " + this->get_hv_name() + " Virtual Machine! Accept only requests from websites that you trust!." _EOL _EOL "Do you want to continue?";

            // Prompt user
            if (!this->confirm(msg)) {
                
                /* Manage throttling */
                if ((getMillis() - this->throttleTimestamp) <= THROTTLE_TIMESPAN) {
                    if (++this->throttleDenies >= THROTTLE_TRIES)
                        this->throttleBlock = true;
                } else {
                    this->throttleDenies = 1;
                    this->throttleTimestamp = getMillis();
                }
                
                if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
                return;
                
            } else {
                
                /* Reset throttle */
                this->throttleDenies = 0;
                this->throttleTimestamp = 0;
                
            }
            
        /* Notify invalid passwords */
        } else if (ans == 2) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
            return;
            
        }
        
        /* Open session */
        HVSession * session = p->hv->sessionOpen(vmName, vmSecret);
        if (session == NULL) {
            if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
            return;
        }
        
        /* Call success callback */
        boost::shared_ptr<CVMWebAPISession> pSession = boost::make_shared<CVMWebAPISession>(p, m_host, session);
        if (IS_CB_AVAILABLE(successCb)) successCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( pSession ));
        
        /* Check if we need a daemon for our current services */
        p->hv->checkDaemonNeed();

    }
    
}

/**
 * Hypervisor installation thread
 */
void CVMWebAPI::thread_install() {
    CVMWebPtr p = this->getPlugin();
    int ans = installHypervisor( FBSTRING_PLUGIN_VERSION, boost::bind(&CVMWebAPI::onInstallProgress, this, _1, _2, _3), p->browserDownloadProvider );
    if (ans == HVE_OK) {
        this->fire_install();
    } else {
        this->fire_installError(hypervisorErrorStr(ans), ans);
    }
};

/**
 * Request a hypervisor installation
 */
int CVMWebAPI::installHV( ) {

    /* If we already have a hypervisor, we don't need to install it */
    if ( hasHypervisor() )
        return HVE_INVALID_STATE;
    
    /* Start installation thread */
    boost::thread t(boost::bind(&CVMWebAPI::thread_install, this ));
    return HVE_SCHEDULED;
    
};

/**
 * Javascript confirmation callback for unsafeConfirm + CVM library
 */
void CVMWebAPI::confirmCallback( const FB::variant& status ) {
    if (!status.is_of_type<bool>()) return;

    // Fetch mapping info
    bool result = status.convert_cast<bool>();

    // Lookup callback info
    {
        boost::lock_guard<boost::mutex> lock(confirmMutex);
        confirmResult = result;
    }
    confirmCond.notify_one();

};

/**
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::unsafeConfirm( std::string msg ) {
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
    
}

/**
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::confirm( std::string msg ) {
    
    #ifdef BROWSER_CONFIRM
    
        return unsafeConfirm( msg );

    #else
    
        CVMWebPtr p = this->getPlugin();
        return CVMConfirmDialog( m_host, p->GetWindow(), msg );

    #endif
    
}

/**
 * Get a access to the idle daemon
 */
FB::variant CVMWebAPI::requestDaemonAccess( const FB::variant &successCb, const FB::variant &failureCb ) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (IS_CB_AVAILABLE(failureCb)) failureCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return CVME_UNSUPPORTED;
    } else {
        if (IS_CB_AVAILABLE(successCb)) successCb.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of(
            boost::make_shared<CVMWebAPIDaemon>(p, m_host)
        ));
        return HVE_SCHEDULED;
    }
}

/**
 * Get a access to the control daemon
 */
FB::variant CVMWebAPI::requestControlAccess( const FB::variant &successCb, const FB::variant &failureCb ) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
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
}

std::string CVMWebAPI::toString() {
    return "[CVMWebAPI]";
}