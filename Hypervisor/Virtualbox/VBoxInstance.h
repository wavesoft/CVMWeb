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
#ifndef VBoxInstance_H
#define VBoxInstance_H

#include "VBoxCommon.h"
#include "VBoxSession.h"

#include <map>

#include "Common/Utilities.h"
#include "Common/Hypervisor.h"
#include "Common/ProgressFeedback.h"
#include "Common/CrashReport.h"
#include "Common/LocalConfig.h"

#include <boost/regex.hpp>

/**
 * VirtualBox Hypervisor
 */
class VBoxInstance : public HVInstance {
public:

    VBoxInstance( std::string fRoot, std::string fBin, std::string fIso ) : HVInstance() {

        // Populate variables
        this->sessionLoaded = false;
        this->hvRoot = fRoot;
        this->hvBinary = fBin;
        this->hvGuestAdditions = fIso;

        // Load hypervisor-specific runtime configuration
        this->hvConfig = LocalConfig::forRuntime("virtualbox");

        // Detect and update VirtualBox Version
        std::vector< std::string > out;
        std::string err;
        this->exec("--version", &out, &err);

        // If we got some output, extract version numbers
        if (out.size() > 0)
            version.set( out[0] );

    };


    /////////////////////////
    // HVInstance Overloads
    /////////////////////////
    virtual int             getType             ( ) { return HV_VIRTUALBOX; };
    virtual int             loadSessions        ( const FiniteTaskPtr & pf = FiniteTaskPtr() );
    virtual bool            waitTillReady       ( const FiniteTaskPtr & pf = FiniteTaskPtr(), const UserInteractionPtr & ui = UserInteractionPtr() );
    virtual HVSessionPtr    allocateSession     ( );
    virtual int             getCapabilities     ( HVINFO_CAPS * caps );
    virtual void            abort               ( );

    /////////////////////////
    // Friend functions
    /////////////////////////
    int                     prepareSession      ( VBoxSession * session );
    std::map<std::string, 
        std::string>        getMachineInfo      ( std::string uuid, int timeout = SYSEXEC_TIMEOUT );
    std::string             getProperty         ( std::string uuid, std::string name );
    std::vector< std::map< std::string, std::string > > 
                            getDiskList         ( );
    std::map<std::string, std::string> 
                            getAllProperties    ( std::string uuid );
    bool                    hasExtPack          ();
    int                     installExtPack      ( const DownloadProviderPtr & downloadProvider, const FiniteTaskPtr & pf = FiniteTaskPtr() );
    HVSessionPtr            sessionByVBID       ( const std::string& virtualBoxGUID );

private:

    /////////////////////////
    // Local properties
    /////////////////////////

    LocalConfigPtr          hvConfig;
    std::string             hvGuestAdditions;
    bool                    sessionLoaded;
    
};

#endif /* end of include guard: VBoxInstance_H */
