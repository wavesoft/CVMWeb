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
#ifndef HVENV_H
#define HVENV_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/regex.hpp> 

#include "CVMGlobals.h"

#include "ProgressFeedback.h"
#include "DownloadProvider.h"
#include "Utilities.h"
#include "CrashReport.h"
#include "ParameterMap.h"
#include "UserInteraction.h"

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
#define STATE_DESTROYED         0
#define STATE_OPPENING          1
#define STATE_OPEN              2
#define STATE_STARTING          3
#define STATE_STARTED           4
#define STATE_ERROR             5
#define STATE_PAUSED            6

/* Extra parameters supported by getExtraInfo() */
#define EXIF_VIDEO_MODE         1

/* Virtual machine session flags */
#define HVF_SYSTEM_64BIT        1       // The system is 64-bit instead of 32-bit
#define HVF_DEPLOYMENT_HDD      2       // Use regular deployment (HDD) instead of micro-iso
#define HVF_GUEST_ADDITIONS     4       // Include a guest additions CD-ROM
#define HVF_FLOPPY_IO           8       // Use floppyIO instead of contextualization CD-ROM
#define HVF_HEADFUL            16       // Start the VM in headful mode
#define HVF_GRAPHICAL          32       // Enable graphical extension (like drag-n-drop)
#define HVF_DUAL_NIC           64       // Use secondary adapter instead of creating a NAT rule on the first one
#define HVF_SERIAL_LOGFILE    128       // Use ttyS0 as external logfile.

/**
 * Shared Pointer Definition
 */
class HVSession;
class HVInstance;
typedef boost::shared_ptr< HVSession >                  HVSessionPtr;
typedef boost::shared_ptr< HVInstance >                 HVInstancePtr;

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
 * Version info
 */
class HypervisorVersion {
public:

    /**
     * The revision information
     */
    int                     major;
    int                     minor;
    int                     build;
    int                     revision;
    std::string             misc;

    /**
     * The version string as extracted from input
     */
    std::string             verString;

    /**
     * Construct from string and automatically populate all the fields
     */
    HypervisorVersion       ( const std::string& verString );

    /**
     * Set a value to the specified version construct
     */
    void                    set( const std::string & version );

    /**
     * Compare to the given revision
     */
    int                     compare( const HypervisorVersion& version );

    /**
     * Compare to the given string
     */
    int                     compareStr( const std::string& version );

    /**
     * Return if a version is defined
     */
    bool                    defined();

private:

    /**
     * Flags if the version is defined
     */
    bool                    isDefined;

};

/**
 * A hypervisor session is actually a VM instance.
 * This is where the actual I/O happens
 */
class HVSession : public boost::enable_shared_from_this<HVSession> {
public:


    /**
     * Session constructor
     * THIS IS A PRIVATE METHOD, YOU SHOULD CALL THE STATIC HVSession::alloc() or HVSession::resume( uuid ) functions!
     *
     * A required parameter is the parameter map of the session.
     */
    HVSession( ParameterMapPtr param, HVInstancePtr hv ) : onDebug(), onOpen(), onStart(), onStop(), onClose(), onError(), onProgress(), parameters(param) {

        // Prepare default parameter values
        parameters->setDefault("cpus",                  "1");
        parameters->setDefault("memory",                "512");
        parameters->setDefault("disk",                  "1024");
        parameters->setDefault("executionCap",          "100");
        parameters->setDefault("apiPort",               BOOST_PP_STRINGIZE( DEFAULT_API_PORT ) );
        parameters->setDefault("flags",                 "0");
        parameters->setDefault("daemonControlled",      "0");
        parameters->setDefault("daemonMinCap",          "0");
        parameters->setDefault("daemonMaxCap",          "0");
        parameters->setDefault("daemonFlags",           "0");
        parameters->setDefault("uuid",                  "");
        parameters->setDefault("ip",                    "");
        parameters->setDefault("key",                   "");
        parameters->setDefault("name",                  "");
        parameters->setDefault("diskURL",               "");
        parameters->setDefault("diskChecksum",          "");
        parameters->setDefault("cernvmVersion",         DEFAULT_CERNVM_VERSION);

        // Open sub-groups
        userData = parameters->subgroup("user-data");
        local = parameters->subgroup("local");
        machine = parameters->subgroup("machine");
        properties = parameters->subgroup("properties");
        
        // Populate local variables
        this->uuid = parameters->get("uuid");
        this->state = parameters->getNum<int>("state", 0);
        this->hypervisor = hv;

    };
    
    ////////////////////////////////////////
    // Session variables
    ////////////////////////////////////////

    std::string             uuid;
    HVInstancePtr           hypervisor;

    /* Currently active user-data */
    //std::map<std::string,
    //    std::string> *      userData;

    std::vector<std::string> overridableVars;
    
    //int                     cpus;
    //int                     memory;
    //int                     disk;
    //int                     executionCap;
    int                     state;
    //int                     apiPort;
    std::string             version;
    std::string             diskChecksum;
    
    //int                     flags;
    int                     pid;
    bool                    editable;
    
    //bool                    daemonControlled;
    //int                     daemonMinCap;
    //int                     daemonMaxCap;
    //int                     daemonFlags;

    int                     internalID;

    ParameterMapPtr         parameters;
    ParameterMapPtr         properties;
    ParameterMapPtr         userData;
    ParameterMapPtr         machine;
    ParameterMapPtr         local;

    ////////////////////////////////////////
    // Session API implementation
    ////////////////////////////////////////

    /**
     * Pause the VM
     */
    virtual int             pause() = 0;

    /**
     * Close the VM
     */
    virtual int             close( bool unmonitored = false ) = 0;

    /**
     * Resume a previously paused VM
     */
    virtual int             resume() = 0;

    /**
     * Cold-boot reset of the VM
     */
    virtual int             reset() = 0;

    /**
     * Power-off the VM
     */
    virtual int             stop() = 0;

    /**
     * Save state of the VM and stop it
     */
    virtual int             hibernate() = 0;

    /**
     * Create or resume session
     */
    virtual int             open() = 0;

    /**
     * Boot the VM
     */    
    virtual int             start( std::map<std::string,std::string> *userData ) = 0;

    /**
     * Change the execution cap
     * The value specified should be between 0 and 100
     */
    virtual int             setExecutionCap(int cap) = 0;

    /**
     * Set an arbitrary property to the VM store
     */
    virtual int             setProperty( std::string name, std::string key ) = 0;

    /**
     * Get an arbitrary property from the VM store
     */
    virtual std::string     getProperty( std::string name ) = 0;

    /**
     * Return the 'hostname:port' address where the user should
     * connect in order to see the RDP display.
     */
    virtual std::string     getRDPAddress() = 0;

    /**
     * Return the IP address where the user should
     * connect in order to interact with the VM.
     */
    virtual std::string     getAPIHost() = 0;

    /**
     * Return the API port number where the user should
     * connect in order to interact with the VM.
     */
    virtual int             getAPIPort() = 0;

    /**
     * Probe the API Port and check if it's alive.
     * There are different handshakes availables:
     *
     * - HSK_NONE   : No protocol negotiation. Just check if we can connect
     * - HSK_SIMPLE : Just send a space and a newline and check if it's still connected
     * - HSK_HTTP   : Send a basic HTTP GET / request and expect some data as response 
     *
     */
    virtual bool            isAPIAlive( unsigned char handshake = HSK_HTTP );

    /**
     * Get extra information from the session that were not thought
     * during the design-phase of the project, or they are hypervisor-specific
     */
    virtual std::string     getExtraInfo( int extraInfo ) = 0;

    /**
     * Re-read the session variables from disk
     */
    virtual int             update() = 0;

    /**
     * Abort current task and prepare session for reaping
     */
    virtual void            abort() = 0;

    callbackDebug           onDebug;
    callbackVoid            onOpen;
    callbackVoid            onStart;
    callbackVoid            onStop;
    callbackVoid            onClose;
    callbackError           onError;
    callbackProgress        onProgress;

};


/**
 * Overloadable base hypervisor class
 */
class HVInstance : public boost::enable_shared_from_this<HVInstance> {
public:

    /**
     * Hypervisor instance constructor
     */    
    HVInstance();
    
    ////////////////////////////////////////
    // Common variables
    ////////////////////////////////////////

    /**
     * The full path to the binary for management to the hypervisor
     */
    std::string             hvBinary;

    /**
     * The root path of the hypervisor
     */
    std::string             hvRoot;

    /**
     * The directory where the VM data should be placed (permanent)
     */
    std::string             dirData;

    /**
     * The directory where the VM data can be placed (volatile)
     */
    std::string             dirDataCache;

    /**
     * HACK: The last STDERR buffer from the exec() function
     */
    std::string             lastExecError;

    /**
     * The hypervisor version
     */
    HypervisorVersion       version;
    
    ////////////////////////////////////////
    // Session management
    ////////////////////////////////////////

    /**
     * A list of currently open sessions
     */
    std::list< HVSessionPtr > openSessions;

    /**
     * The map of session UUIDs and their object instance
     */
    std::map< std::string, HVSessionPtr >      sessions;

    /**
     * Return a session by it's name
     */
    HVSessionPtr            sessionByName       ( const std::string& name );

    /**
     * Open a session using the specified input parameters
     */
    virtual HVSessionPtr    sessionOpen         ( const ParameterMapPtr& parameters, const FiniteTaskPtr& pf );

    /**
     * Validate a session using the specified input parameters
     */
    virtual int             sessionValidate     ( const ParameterMapPtr& parameters );

    ////////////////////////////////////////
    // Overridable functions
    ////////////////////////////////////////

    /**
     * Return the hypervisor type ID
     */
    virtual int             getType             ( ) { return HV_NONE; };

    /**
     * Load the sessions from disk/hypervisor into the sessions[] map
     */
    virtual int             loadSessions        ( const FiniteTaskPtr & pf = FiniteTaskPtr() ) = 0;

    /**
     * Allocate a new session and store it on the openSessions[] list and the sessions[] map
     */
    virtual HVSessionPtr    allocateSession     ( ) = 0;

    /**
     * Fetch the hypervisor capabilities
     */
    virtual int             getCapabilities     ( HVINFO_CAPS * caps ) = 0;

    /**
     * Wait until the hypervisor is initialized
     */
    virtual bool            waitTillReady       ( const FiniteTaskPtr & pf = FiniteTaskPtr(), const UserInteractionPtr & ui = UserInteractionPtr() ) = 0;

    /**
     * Count the resources used by the hypervisor
     */
    virtual int             getUsage            ( HVINFO_RES * usage);

    /**
     * Immediately abort current task and reap all sessions
     */
    virtual void            abort() = 0;

    //////////////////////////////////////////////
    // Tool functions
    // (Used internally or from session objects)
    //////////////////////////////////////////////

    /**
     * Execute the hypervisor binary, appending the specified argument list.
     * This command just encapsulates a sysExec function
     */
    int                     exec                ( std::string args, std::vector<std::string> * stdoutList, std::string * stderrMsg, const SysExecConfig& config );

    /**
     * Download an arbitrary file and validate it against a checksum
     * file, both provided as URLs
     */
    int                     downloadFileURL     ( const std::string & fileURL, const std::string & checksumURL, std::string * filename, const FiniteTaskPtr & pf = FiniteTaskPtr(), const int retries = 2 );

    /**
     * Download an arbitrary file and validate it against a checksum
     * string specified in parameter
     */
    int                     downloadFile        ( const std::string & fileURL, const std::string & checksumString, std::string * filename, const FiniteTaskPtr & pf = FiniteTaskPtr(), const int retries = 2 );
    
    /**
     * Download a gzip-compressed arbitrary file and validate it's extracted
     * contents against a checksum string specified in parameter
     */
    int                     downloadFileGZ      ( const std::string & fileURL, const std::string & checksumString, std::string * filename, const FiniteTaskPtr & pf = FiniteTaskPtr(), const int retries = 2 );


    /**
     * Return the cached disk image for the specified CernVM version
     */
    int                     cernVMCached        ( std::string version, std::string * filename );

    /**
     * Parse the given filename and detect the CernVM Version
     */
    std::string             cernVMVersion       ( std::string filename );

    /**
     * Build a Contextualization CD-ROM with the specified user-data and
     * store it to the file pointer specified.
     */
    int                     buildContextISO     ( std::string userData, std::string * filename );

    /**
     * Build a floppy disk using the specified user-data and store the resulting
     * filename to the filename pointer.
     */
    int                     buildFloppyIO       ( std::string userData, std::string * filename );
    

    //////////////////////////////////////////////
    // Control functions
    //////////////////////////////////////////////

    /**
     * Check if we need a daemon for our sessions and if we do, start it.
     * Otherwise stop any running instance.
     */
    int                     checkDaemonNeed ();

    /**
     * Change the default download provider to the one specified
     */
    void                    setDownloadProvider( DownloadProviderPtr p );

    /**
     * Change the default user interaction proxy to the one specified
     */
    void                    setUserInteraction( UserInteractionPtr p );

    /* HACK: Only the JSAPI knows where it's located. Therefore it must provide it to
             the Hypervisor class in order to use the checkDaemonNeed() function. It's
             a hack because those two systems (JSAPI & HypervisorAPI) should be isolated. */
    std::string             daemonBinPath;
    
protected:
    int                     sessionID;
    DownloadProviderPtr     downloadProvider;
    UserInteractionPtr      userInteraction;
};

//////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////

/**
 * Detect the installed hypervisors in the system and return the first
 * available hypervisor instance pointer.
 */
HVInstancePtr                   detectHypervisor    ( );

/**
 * Install the default hypervisor in the system
 */
int                             installHypervisor   ( const DownloadProviderPtr & downloadProvider, 
                                                      const UserInteractionPtr & ui = UserInteractionPtr(), 
                                                      const FiniteTaskPtr & pf = FiniteTaskPtr(), 
                                                      int retries = 4 );

/**
 * Return the string representation of a hypervisor error code
 */
std::string                     hypervisorErrorStr  ( int error );


#endif /* end of include guard: HVENV_H */
