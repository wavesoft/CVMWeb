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

#ifndef HVENV_H
#define HVENV_H

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

/* Hypervisor types */
#define HV_VIRTUALBOX           1

/* Error messages */
#define HVE_SHEDULED            1
#define HVE_OK                  0
#define HVE_CREATE_ERROR        -1
#define HVE_MODIFY_ERROR        -2
#define HVE_CONTROL_ERROR       -3
#define HVE_DELETE_ERROR        -4
#define HVE_QUERY_ERROR         -5
#define HVE_IO_ERROR            -6
#define HVE_EXTERNAL_ERROR      -7
#define HVE_INVALID_STATE       -8
#define HVE_NOT_FOUND           -9
#define HVE_NOT_IMPLEMENTED     -100

/* Session states */
#define STATE_CLOSED            0
#define STATE_OPPENING          1
#define STATE_OPEN              2
#define STATE_STARTING          3
#define STATE_STARTED           4
#define STATE_ERROR             5
#define STATE_PAUSED            6

/* Default CernVM Version */
#define DEFAULT_CERNVM_VERSION  "1.3"

class HVSession {
public:
    
    HVSession() {
        this->onProgress = NULL;
        this->onOpen = NULL;
        this->onStart = NULL;
        this->onClose = NULL;
        this->onError = NULL;
        this->onLive = NULL;
        this->onDead = NULL;
        this->onStop = NULL;
        this->onDebug = NULL;
    };
    
    std::string             ip;
    std::string             key;
    std::string             name;
    
    int                     cpus;
    int                     memory;
    int                     disk;
    int                     executionCap;
    int                     state;
    std::string             version;

    int                     internalID;
        
    virtual int             pause();
    virtual int             close();
    virtual int             resume();
    virtual int             reset();
    virtual int             stop();
    virtual int             open( int cpus, int memory, int disk, std::string cvmVersion );
    virtual int             start( std::string userData );
    virtual int             setExecutionCap(int cap);
    virtual int             setProperty( std::string name, std::string key );
    virtual std::string     getProperty( std::string name );

    void *                  cbObject;
    void (*onProgress)      (int, int, std::string, void *);
    void (*onError)         (std::string, int, std::string, void *);
    void (*onDebug)         (std::string, void *);
    void (*onOpen)          (void *);
    void (*onStart)         (void *);
    void (*onStop)          (void *);
    void (*onClose )        (void *);
    void (*onLive )         (void *);
    void (*onDead )         (void *);
    
};

class Hypervisor {
public:
    
    Hypervisor();
    int                     type;
    
    int                     verMajor;
    int                     verMinor;
    std::string             verString;
    std::string             hvBinary;
    std::string             hvRoot;
    std::string             dirData;
    std::string             dirDataCache;
        
    /* Session management commands */
    std::vector<HVSession*> sessions;
    HVSession *             sessionOpen     ( std::string name, std::string key );
    HVSession *             sessionGet      ( int id );
    int                     sessionFree     ( int id );

    /* Overridable functions */
    virtual int             loadSessions    ( );
    virtual HVSession *     allocateSession ( std::string name, std::string key );
    virtual int             freeSession     ( HVSession * sess );
    virtual int             registerSession ( HVSession * sess );
    
    /* Tool functions */
    int                     exec            ( std::string args, std::vector<std::string> * stdout );
    void                    detectVersion   ( );
    int                     cernVMDownload  ( std::string version, std::string * filename );
    int                     cernVMCached    ( std::string version, std::string * filename );
    std::string             cernVMVersion   ( std::string filename );
    int                     buildContextISO ( std::string userData, std::string * filename );
    std::string             getTmpFile      ( std::string suffix );
    
private:
    int                     sessionID;
    
};

Hypervisor *                detectHypervisor        ();
std::string                 hypervisorErrorStr      ( int error );

template <typename T> T ston( const std::string &Text );

#endif /* end of include guard: HVENV_H */
