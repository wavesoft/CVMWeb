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
 * Forward progress events
 */
void __fwProgress( int step, int total, std::string msg, void * ptr ) {
    CVMWebAPI * self = (CVMWebAPI * ) ptr;
    self->fire_installProgress( step, total, msg );
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
bool CVMWebAPI::isDomainPriviledged() {
    std::string domain = this->getDomainName();
    
    /* Domain is empty only when we see the plugin from a file:// URL
     * (And yes, even localhost is not considered priviledged) */
    return domain.empty();
}

/**
 * Create and return a session object. 
 */
FB::variant CVMWebAPI::requestSession( const FB::variant& vm, const FB::variant& secret, const FB::JSObjectPtr &successCb, const FB::JSObjectPtr &failureCb ) {

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

void CVMWebAPI::requestSession_thread( const FB::variant& vm, const FB::variant& secret, const FB::JSObjectPtr &successCb, const FB::JSObjectPtr &failureCb ) {
    
    /* Fetch domain info once again */
    std::string domain = this->getDomainName();

    /*
    // Try to update authorized keystore if it's in an invalid state
    if (!localDomain) {
        // Trigger update
        if (!this->crypto->valid)
            this->crypto->updateAuthorizedKeystore();

        // Still invalid? Something's wrong
        if (!this->crypto->valid) {
            if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_NOT_VALIDATED ));
            return;
        }

        // Block requests from untrusted domains
        if (!this->crypto->isDomainValid(domain)) {
            if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_NOT_TRUSTED ));
            return;
        }
    }
    */
        
    /* Handle request */
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return;

    } else {

        /* Fetch info */
        std::string vmName = vm.convert_cast<std::string>();
        std::string vmSecret = secret.convert_cast<std::string>();
        
        /* Try to open the session */
        int ans = p->hv->sessionValidate( vmName, vmSecret );
        
        /* Notify user that a new session will open */
        if (ans == 0) {

            // Newline-specific split
            #ifdef _WIN32
            std::string msg = "The website " + domain + " is trying to allocate a " + this->get_hv_name() + " Virtual Machine! If you were not prompted for such action by the website please reject this request.\r\n\r\nDo you want to continue?";
            #else
            std::string msg = "The website " + domain + " is trying to allocate a " + this->get_hv_name() + " Virtual Machine! If you were not prompted for such action by the website please reject this request.\n\nDo you want to continue?";
            #endif

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
                
                if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
                return;
                
            } else {
                
                /* Reset throttle */
                this->throttleDenies = 0;
                this->throttleTimestamp = 0;
                
            }
            
        /* Notify invalid passwords */
        } else if (ans == 2) {
            if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
            return;
            
        }
        
        /* Open session */
        HVSession * session = p->hv->sessionOpen(vmName, vmSecret);
        if (session == NULL) {
            if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_PASSWORD_DENIED ));
            return;
        }
        
        /* Call success callback */
        boost::shared_ptr<CVMWebAPISession> pSession = boost::make_shared<CVMWebAPISession>(p, m_host, session);
        if (successCb != NULL) successCb->InvokeAsync("", FB::variant_list_of( pSession ));
        
        /* Check if we need a daemon for our current services */
        p->hv->checkDaemonNeed();

    }
    
}

/**
 * Hypervisor installation thread
 */
void CVMWebAPI::thread_install() {
    CVMWebPtr p = this->getPlugin();
    int ans = installHypervisor( FBSTRING_PLUGIN_VERSION, &__fwProgress, this );
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
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::confirm( std::string msg ) {
    
    #ifdef BROWSER_CONFIRM
    
        // Retrieve a reference to the DOM Window
        FB::DOM::WindowPtr window = m_host->getDOMWindow();
    
        // Check if the DOM Window has an alert property
        if (window && window->getJSObject()->HasProperty("window")) {
        
            // Create a reference to alert
            FB::JSObjectPtr obj = window->getProperty<FB::JSObjectPtr>("window");
        
            // Make sure the function is valid native function and not a hack 
            FB::variant f = obj->GetProperty("confirm");
            FB::JSObjectPtr fPtr = f.convert_cast<FB::JSObjectPtr>();
            std::string fType = fPtr->Invoke("toString", FB::variant_list_of( msg )).convert_cast<std::string>();
            if (fType.find("native") == std::string::npos)
                return false;
        
            // Invoke alert with some text
            return obj->Invoke("confirm", FB::variant_list_of( msg )).convert_cast<bool>();
        } else {
            return false;
        }

    #else
    
        CVMWebPtr p = this->getPlugin();
        return CVMConfirmDialog( m_host, p->GetWindow(), msg );

    #endif
    
    
}

/**
 * Get a access to the idle daemon
 */
FB::variant CVMWebAPI::requestDaemonAccess( const FB::JSObjectPtr &successCb, const FB::JSObjectPtr &failureCb ) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return CVME_UNSUPPORTED;
    } else {
        if (successCb != NULL) successCb->InvokeAsync("", FB::variant_list_of(
            boost::make_shared<CVMWebAPIDaemon>(p, m_host)
        ));
        return HVE_SCHEDULED;
    }
}

/**
 * Get a access to the control daemon
 */
FB::variant CVMWebAPI::requestControlAccess( const FB::JSObjectPtr &successCb, const FB::JSObjectPtr &failureCb ) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_UNSUPPORTED ));
        return CVME_UNSUPPORTED;
    } else {
        if (!isDomainPriviledged()) {
            if (failureCb != NULL) failureCb->InvokeAsync("", FB::variant_list_of( CVME_ACCESS_DENIED ));
            return CVME_ACCESS_DENIED;
        } else {
            if (successCb != NULL) successCb->InvokeAsync("", FB::variant_list_of(
                boost::make_shared<CVMWebLocalConfig>(p, m_host)
            ));
            return HVE_SCHEDULED;
        }
    }
}

std::string CVMWebAPI::toString() {
    return "[CVMWebAPI]";
}