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
#include "global/config.h"

#include "CVMWebAPI.h"
#include "CVMWebAPISession.h"

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

FB::variant CVMWebAPI::requestSession(const FB::variant& vm, const FB::variant& secret) {
    CVMWebPtr p = this->getPlugin();
    if (p->hv == NULL) {
        return false;
    } else {
        std::string vmName = vm.cast<std::string>();
        std::string vmSecret = secret.cast<std::string>();
        HVSession * session = p->hv->sessionOpen(vmName, vmSecret);
        if (session == NULL) return false;
        return boost::make_shared<CVMWebAPISession>(p, m_host, session);
    }
}
