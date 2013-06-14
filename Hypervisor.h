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

#include "Utilities.h"

/* Hypervisor types */
#define HV_NONE                 0
#define HV_VIRTUALBOX           1

/* Error messages */
#define HVE_ALREADY_EXISTS      2
#define HVE_SCHEDULED           1
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
#define HVE_NOT_ALLOWED         -10
#define HVE_NOT_SUPPORTED       -11
#define HVE_NOT_VALIDATED       -12
#define HVE_NOT_TRUSTED         -13
#define HVE_STILL_WORKING       -14
#define HVE_USAGE_ERROR         -99
#define HVE_NOT_IMPLEMENTED     -100

/* Session states */
#define STATE_CLOSED            0
#define STATE_OPPENING          1
#define STATE_OPEN              2
#define STATE_STARTING          3
#define STATE_STARTED           4
#define STATE_ERROR             5
#define STATE_PAUSED            6

/* Virtual machine session flags */
#define HVF_SYSTEM_64BIT        1       // The system is 64-bit instead of 32-bit
#define HVF_DEPLOYMENT_HDD      2       // Use regular deployment (HDD) instead of micro-iso
#define HVF_GUEST_ADDITIONS     4       // Include a guest additions CD-ROM
#define HVF_FLOPPY_IO           8       // Use floppyIO instead of contextualization CD-ROM

/* Default CernVM Version */
#define DEFAULT_CERNVM_VERSION  "1.4"
#define DEFAULT_API_PORT        80

/**
 * A hypervisor session is actually a VM instance.
 * This is where the actual I/O happens
 */
class HVSession {
public:
    
    HVSession() {
        this->onProgress = NULL;
        this->onOpen = NULL;
        this->onStart = NULL;
        this->onClose = NULL;
        this->onError = NULL;
        this->onStop = NULL;
        this->onDebug = NULL;
        
        this->cpus = 1;
        this->memory = 256;
        this->disk = 1024;
        this->executionCap = 100;
        this->state = 0;
        this->internalID = 0;
        this->cbObject = NULL;
        this->uuid = "";
        this->ip = "";
        this->key = "";
        this->name = "";
        this->apiPort = DEFAULT_API_PORT;
        this->flags = 0;
        this->userData = "";
        this->diskChecksum = "";
        
        this->daemonControlled = false;
        this->daemonMinCap = 0;
        this->daemonMaxCap = 100;
        this->daemonFlags = 0;
        
        
    };
    
    std::string             uuid;
    std::string             ip;
    std::string             key;
    std::string             name;
    std::string             userData;
    
    int                     cpus;
    int                     memory;
    int                     disk;
    int                     executionCap;
    int                     state;
    int                     apiPort;
    std::string             version;
    std::string             diskChecksum;
    
    int                     flags;
    
    bool                    daemonControlled;
    int                     daemonMinCap;
    int                     daemonMaxCap;
    int                     daemonFlags;

    int                     internalID;
        
    virtual int             pause();
    virtual int             close();
    virtual int             resume();
    virtual int             reset();
    virtual int             stop();
    virtual int             hibernate();
    virtual int             open( int cpus, int memory, int disk, std::string cvmVersion, int flags );
    virtual int             start( std::map<std::string,std::string> *userData );
    virtual int             setExecutionCap(int cap);
    virtual int             setProperty( std::string name, std::string key );
    virtual std::string     getProperty( std::string name );
    virtual std::string     getIP();
    virtual std::string     getRDPHost();
    virtual bool            isAPIAlive();

    virtual int             update();

    void *                  cbObject;
    void (*onProgress)      (int, int, std::string, void *);
    void (*onError)         (std::string, int, std::string, void *);
    void (*onDebug)         (std::string, void *);
    void (*onOpen)          (void *);
    void (*onStart)         (void *);
    void (*onStop)          (void *);
    void (*onClose )        (void *);
    
};

/**
 * Resource information structure
 */
typedef struct {
    
    int         cpus;   // Maximum or currently used number of CPUs
    int         memory; // Maximum or currently used RAM size (MBytes)
    long int    disk;   // Maximum or currently used disk size (MBytes)
    
} HVINFO_RES;

/**
 * CPUID information
 */
typedef struct {
    char            vendor[13]; // Vendor string + Null Char
    int             featuresA;  // Raw feature flags from EAX=1/EDX
    int             featuresB;  // Raw feature flags from EAX=1/ECX
    int             featuresC;  // Raw feature flags from EAX=80000001h/EDX
    int             featuresD;  // Raw feature flags from EAX=80000001h/ECX
    
    bool            hasVT;      // Hardware virtualization
    bool            hasVM;      // Memory virtualization (nested page tables)
    bool            has64bit;   // Is the 64-bit instruction set supported?
    
    unsigned char   stepping;   // CPU Stepping
    unsigned char   model;      // CPU Model
    unsigned char   family;     // CPU Family
    unsigned char   type;       // CPU Type
    unsigned char   exmodel;    // CPU Extended Model
    unsigned char   exfamily;   // CPU Extended Family
    
} HVINFO_CPUID;

/**
 * Capabilities information
 */
typedef struct {
    
    HVINFO_RES          max;        // Maximum available resources
    HVINFO_CPUID        cpu;        // CPU information
    bool                isReady;    // Current configuration allows VMs to start without problems
    
} HVINFO_CAPS;

/**
 * Overloadable base hypervisor class
 */
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
    HVSession *             sessionLocate       ( std::string uuid );
    HVSession *             sessionOpen         ( const std::string & name, const std::string & key );
    HVSession *             sessionGet          ( int id );
    int                     sessionFree         ( int id );
    int                     sessionValidate     ( std::string name, std::string key );

    /* Overridable functions */
    virtual int             loadSessions        ( );
    virtual int             updateSession       ( HVSession * session );
    virtual HVSession *     allocateSession     ( std::string name, std::string key );
    virtual int             freeSession         ( HVSession * sess );
    virtual int             registerSession     ( HVSession * sess );
    virtual int             getUsage            ( HVINFO_RES * usage);
    virtual int             getCapabilities     ( HVINFO_CAPS * caps );
    virtual bool            waitTillReady       ( );
    
    /* Tool functions (used internally or from session objects) */
    int                     exec                ( std::string args, std::vector<std::string> * stdoutList );
    void                    detectVersion       ( );
    int                     cernVMDownload      ( std::string version, std::string * filename, HVPROGRESS_FEEDBACK * feedback );
    int                     cernVMCached        ( std::string version, std::string * filename );
    std::string             cernVMVersion       ( std::string filename );
    int                     diskImageDownload   ( std::string url, std::string checksum, std::string * filename, HVPROGRESS_FEEDBACK * fb );
    int                     buildContextISO     ( std::string userData, std::string * filename );
    int                     buildFloppyIO       ( std::string userData, std::string * filename );
    
    /* Control functions (called externally) */
    int                     checkDaemonNeed ();
    void                    setDownloadProvider( DownloadProvider * p );
    
    /* HACK: Only the JSAPI knows where it's located. Therefore it must provide it to
             the Hypervisor class in order to use the checkDaemonNeed() function. It's
             a hack because those two systems (JSAPI & HypervisorAPI) should be isolated. */
    std::string             daemonBinPath;
     
    
private:
    int                     sessionID;
    DownloadProvider *      downloadProvider;
    
};

/**
 * Exposed functions
 */
Hypervisor *                    detectHypervisor    ( );
void                            freeHypervisor      ( Hypervisor * );
int                             installHypervisor   ( std::string clientVersion, void(*cbProgress)(int, int, std::string, void*), void * cbData, DownloadProvider * provider );
std::string                     hypervisorErrorStr  ( int error );


#endif /* end of include guard: HVENV_H */
