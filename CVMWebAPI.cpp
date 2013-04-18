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

#include "DialogManager.h"
#include "DialogManagerMac.h"

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
 * Create and return a session object. 
 */
FB::variant CVMWebAPI::requestSession(const FB::variant& vm, const FB::variant& secret) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        return CVME_UNSUPPORTED;
    } else {

        /* Fetch info */
        std::string vmName = vm.cast<std::string>();
        std::string vmSecret = secret.cast<std::string>();
        
        /* Try to open the session */
        int ans = p->hv->sessionValidate( vmName, vmSecret );
        
        /* Notify user that a new session will open */
        if (ans == 0) {
            std::string msg = "The website " + this->getDomainName() + " is trying to allocate a " + this->get_hv_name() + " Virtual Machine. Do you want to allow it?";
            if (!this->confirm(msg)) return CVME_ACCESS_DENIED;
        
        /* Notify invalid passwords */
        } else if (ans == 2) {
            return CVME_PASSWORD_DENIED;
        }
        
        /* Open session */
        HVSession * session = p->hv->sessionOpen(vmName, vmSecret);
        if (session == NULL) return CVME_PASSWORD_DENIED;
        return boost::make_shared<CVMWebAPISession>(p, m_host, session);
    }
}

/**
 * Show a confirmation dialog using browser's API
 */
bool CVMWebAPI::confirm( std::string msg ) {
    
    DialogManager * i = DialogManagerMac::get();
    CVMWebPtr p = this->getPlugin();
    return i->ConfirmDialog( m_host, p->GetWindow(), msg );;
    
    /*
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
        obj->Invoke("alert", FB::variant_list_of( fType ));
        
        // Invoke alert with some text
        return obj->Invoke("confirm", FB::variant_list_of( msg )).convert_cast<bool>();
    } else {
        return false;
    }
    */
    
}