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

#include <boost/regex.hpp>

/**
 * Shared Pointer Definition
 */
class VBoxInstance;
typedef boost::shared_ptr< VBoxInstance >     VBoxInstancePtr;

/**
 * VirtualBox Hypervisor
 */
class VBoxInstance : public HVInstance {
public:

    VBoxInstance( std::string fRoot, std::string fBin, std::string fIso ) : HVInstance() {
        this->sessionLoaded = false;
        this->hvRoot = fRoot;
        this->hvBinary = fBin;
        this->hvGuestAdditions = fIso;
    };


    /////////////////////////
    // HVInstance Overloads
    /////////////////////////
    virtual int             loadSessions        ( const FiniteTaskPtr & pf = FiniteTaskPtr() );
    virtual HVSessionPtr    allocateSession     ( );
    virtual int             getCapabilities     ( HVINFO_CAPS * caps );
    virtual bool            waitTillReady       ( const FiniteTaskPtr & pf = FiniteTaskPtr() );

    /////////////////////////
    // Friend functions
    /////////////////////////
    int                     prepareSession      ( VBoxSession * session );
    std::map<std::string, 
        std::string>        getMachineInfo      ( std::string uuid, int timeout = SYSEXEC_TIMEOUT );
    std::string             getProperty         ( std::string uuid, std::string name );
    std::vector< std::map< std::string, std::string > > 
                            getDiskList         ();
    std::map<std::string, std::string> 
                            getAllProperties    ( std::string uuid );
    bool                    hasExtPack          ();
    int                     installExtPack      ( std::string versionID, DownloadProviderPtr downloadProvider, const FiniteTaskPtr & pf = FiniteTaskPtr() );
    HVSessionPtr            sessionByVBID       ( const std::string& virtualBoxGUID );

private:

    /////////////////////////
    // Local properties
    /////////////////////////

    std::string             hvGuestAdditions;
    bool                    sessionLoaded;
    
};

#endif /* end of include guard: VBoxInstance_H */
