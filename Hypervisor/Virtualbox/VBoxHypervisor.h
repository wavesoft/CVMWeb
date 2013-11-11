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

#pragma once
#ifndef VBOXHYPERVISOR_H
#define VBOXHYPERVISOR_H

#include "VBoxCommon.h"
#include "VBoxSession.h"

#include <map>

#include "Common/Utilities.h"
#include "Common/Hypervisor.h"
#include "Common/CrashReport.h"

#include <boost/regex.hpp>

/**
 * VirtualBox Hypervisor
 */
class VBoxHypervisor : public Hypervisor {
public:

    VBoxHypervisor() : Hypervisor() {
        this->sessionLoaded = false;
    };

    std::string             hvGuestAdditions;

    bool                    hasExtPack          ();
    int                     installExtPack      ( std::string versionID, DownloadProviderPtr downloadProvider, callbackProgress cbProgress, int progressMin = 0, int progressMax = 100, int progressTotal = 100 );

    /* Internal parameters */
    int                     prepareSession      ( VBoxSession * session );
    std::map<std::string, 
        std::string>        getMachineInfo      ( std::string uuid, int timeout = SYSEXEC_TIMEOUT );
    std::string             getProperty         ( std::string uuid, std::string name );
    std::vector< std::map< std::string, std::string > > getDiskList();
    std::map<std::string, std::string> getAllProperties  ( std::string uuid );

    /* Overloads */
    virtual int             loadSessions        ( );
    virtual int             updateSession       ( HVSession * session, bool fast );
    virtual HVSession *     allocateSession     ( std::string name, std::string key );
    virtual int             getCapabilities     ( HVINFO_CAPS * caps );
    virtual bool            waitTillReady       ( std::string pluginVersion, callbackProgress progress = 0, int progressMin = 0, int progressMax = 100, int progressTotal = 100 );

private:
    bool                    sessionLoaded;
    
};

#endif /* end of include guard: VBOXHYPERVISOR_H */
