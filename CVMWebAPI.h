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
#include "CVMWeb.h"

#ifndef H_CVMWebAPI
#define H_CVMWebAPI

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
        registerMethod("requestSession",    make_method(this, &CVMWebAPI::requestSession));

        // Read-only property
        registerProperty("version", make_property(this, &CVMWebAPI::get_version));
        registerProperty("hypervisorName", make_property(this, &CVMWebAPI::get_hv_name));
        registerProperty("hypervisorVersion", make_property(this, &CVMWebAPI::get_hv_version));
        
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// @fn CVMWebAPI::~CVMWebAPI()
    ///
    /// @brief  Destructor.  Remember that this object will not be released until
    ///         the browser is done with it; this will almost definitely be after
    ///         the plugin is released.
    ///////////////////////////////////////////////////////////////////////////////
    virtual ~CVMWebAPI() {};

    CVMWebPtr getPlugin();

    // Read-only property ${PROPERTY.ident}
    std::string get_version();
    std::string get_hv_name();
    std::string get_hv_version();

    // Methods
    FB::variant requestSession(const FB::variant& vm, const FB::variant& code);
    
private:
    CVMWebWeakPtr m_plugin;
    FB::BrowserHostPtr m_host;

};

#endif // H_CVMWebAPI


/*** -- CODE BIN --
        
    // Read-write property
    registerProperty("testString",
                     make_property(this,
                                   &CVMWebAPI::get_testString,
                                   &CVMWebAPI::set_testString));

    // Read/Write property ${PROPERTY.ident}
    std::string get_testString();
    void set_testString(const std::string& val);

    std::string m_testString;

****/
