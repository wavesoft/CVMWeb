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

#ifndef CVMWEBLOCALCONFIG_H_W7H2YROC
#define CVMWEBLOCALCONFIG_H_W7H2YROC

#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "Timer.h"
#include "BrowserHost.h"
#include "CVMWeb.h"
#include "Hypervisor.h"
 #include "CrashReport.h"

class CVMWebLocalConfig : public FB::JSAPIAuto
{
public:

    CVMWebLocalConfig(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host) :
    m_plugin(plugin), m_host(host)
    {
        
        // Beautification
        registerMethod("toString",          make_method(this, &CVMWebLocalConfig::toString));

    }

    virtual                 ~CVMWebLocalConfig() {};
    CVMWebPtr               getPlugin();
    std::string             toString();


private:
    CVMWebWeakPtr           m_plugin;
    FB::BrowserHostPtr      m_host;

    std::string             getDomainName();
    bool                    isDomainPrivileged();

};

#endif /* end of include guard: CVMWEBLOCALCONFIG_H_W7H2YROC */
