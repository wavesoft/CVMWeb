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

#ifndef VIRTUALBOX_H
#define VIRTUALBOX_H

#include "Hypervisor.h"
#include "CrashReport.h"
#include <map>
#include <boost/regex.hpp>

/* Forward decleration */
class Virtualbox;

/**
 * VirtualBox Session
 */
class VBoxSession : public HVSession {
public:
    
    Virtualbox *            host;
    int                     rdpPort;
    int                     localApiPort;
    
    virtual int             pause               ();
    virtual int             close               ( bool unmonitored = false );
    virtual int             resume              ();
    virtual int             reset               ();
    virtual int             stop                ();
    virtual int             hibernate           ();
    virtual int             open                ( int cpus, int memory, int disk, std::string cvmVersion, int flags );
    virtual int             start               ( std::map<std::string,std::string> *userData );
    virtual int             setExecutionCap     ( int cap);
    virtual int             setProperty         ( std::string name, std::string key );
    virtual std::string     getProperty         ( std::string name, bool forceUpdate = false );
    virtual std::string     getRDPHost          ();
    virtual std::string     getExtraInfo        ( int extraInfo );
    virtual std::string     getAPIHost          ();
    virtual int             getAPIPort          ();

    virtual int             update              ();
    virtual int             updateFast          ();
    
    /* VirtualBox-specific functions */
    int                     wrapExec            ( std::string cmd, std::vector<std::string> * stdoutList, std::string * stderrMsg = NULL, int retries = 4, int timeout = SYSEXEC_TIMEOUT );
    int                     getMachineUUID      ( std::string mname, std::string * ans_uuid,  int flags );
    std::string             getDataFolder       ();
    std::string             getHostOnlyAdapter  ();
    std::map<std::string, 
        std::string>        getMachineInfo      ( int timeout = SYSEXEC_TIMEOUT );
    int                     startVM             ();
    int                     controlVM           ( std::string how, int timeout = SYSEXEC_TIMEOUT );

    std::string             dataPath;
    bool                    updateLock;
    
    /* Offline properties map (for optimizing performance) */
    std::map<
        std::string,
        std::string >       properties;
    std::map<
        std::string,
        std::string >       unsyncedProperties;
    
};

/**
 * VirtualBox Hypervisor
 */
class Virtualbox : public Hypervisor {
public:

    Virtualbox() : Hypervisor() {
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

#endif /* end of include guard: VIRTUALBOX_H */
