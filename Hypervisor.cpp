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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cmath>

#include "Utilities.h"
#include "Hypervisor.h"
#include "Virtualbox.h"
#include "DaemonCtl.h"

#include "contextiso.h"

using namespace std;

/* Incomplete type placeholders */
int Hypervisor::loadSessions()                                      { return HVE_NOT_IMPLEMENTED; }
int Hypervisor::getCapabilities ( HVINFO_CAPS * )                   { return HVE_NOT_IMPLEMENTED; };
int HVSession::pause()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::close()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::stop()                                               { return HVE_NOT_IMPLEMENTED; }
int HVSession::resume()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::hibernate()                                          { return HVE_NOT_IMPLEMENTED; }
int HVSession::reset()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::open( int cpus, int memory, int disk, std::string cvmVersion ) 
                                                                    { return HVE_NOT_IMPLEMENTED; }
int HVSession::start( std::string userData )                        { return HVE_NOT_IMPLEMENTED; }
int HVSession::setExecutionCap(int cap)                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::setProperty( std::string name, std::string key )     { return HVE_NOT_IMPLEMENTED; }
std::string HVSession::getProperty( std::string name )              { return ""; }
std::string HVSession::getRDPHost()                                 { return ""; }

/** 
 * Return the string representation of the given error code
 */
std::string hypervisorErrorStr( int error ) {
    if (error == 0) return "No error";
    if (error == 1) return "Sheduled";
    if (error == -1) return "Creation error";
    if (error == -2) return "Modification error";
    if (error == -3) return "Control error";
    if (error == -4) return "Delete error";
    if (error == -5) return "Query error";
    if (error == -6) return "I/O error";
    if (error == -7) return "External error";
    if (error == -8) return "Not in a valid state";
    if (error == -9) return "Not found";
    if (error == -10) return "Not allowed";
    if (error == -11) return "Not supported";
    if (error == -12) return "Not validated";
    if (error == -20) return "Password denied";
    if (error == -100) return "Not implemented";
    return "Unknown error";
};

/**
 * Return the IP Address of the session
 */
std::string HVSession::getIP() {
    if (this->ip.empty()) {
        std::string guessIP = this->getProperty("/VirtualBox/GuestInfo/Net/1/V4/IP");
        if (!guessIP.empty()) {
            this->ip = guessIP;
            return guessIP;
        } else {
            return "";
        }
    } else {
        return this->ip;
    }
}

/**
 * Try to connect to the API port and check if it succeeded
 */

bool HVSession::isAPIAlive() {
    std::string ip = this->getIP();
    if (ip.empty()) return false;
    return isPortOpen( ip.c_str(), this->apiPort );
}

/**
 * Measure the resources from the sessions
 */
int Hypervisor::getUsage( HVINFO_RES * resCount ) { 
    resCount->memory = 0;
    resCount->cpus = 0;
    resCount->disk = 0;
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        resCount->memory += sess->memory;
        resCount->cpus += sess->cpus;
        resCount->disk += sess->disk;
    }
    return HVE_OK;
}


/**
 * Use LibcontextISO to create a cd-rom for this VM
 */
int Hypervisor::buildContextISO ( std::string userData, std::string * filename ) {
    ofstream isoFile;
    string iso = getTmpFile(".iso");
    
    string ctxFileContents = base64_encode( userData );
    ctxFileContents = "EC2_USER_DATA=\"" +ctxFileContents + "\"\nONE_CONTEXT_PATH=\"/var/lib/amiconfig\"\n";
    const char * fData = ctxFileContents.c_str();
    
    char * data = build_simple_cdrom( "CONTEXT_INFO", "CONTEXT.SH", fData, ctxFileContents.length() );
    isoFile.open( iso.c_str(), std::ios_base::out | std::ios_base::binary );
    if (!isoFile.fail()) {
        isoFile.write( data, CONTEXTISO_CDROM_SIZE );
        isoFile.close();
        *filename = iso;
        return HVE_OK;
    } else {
        return HVE_IO_ERROR;
    }
};

/**
 * Get the CernVM version of the filename specified
 */
std::string Hypervisor::cernVMVersion( std::string filename ) {
    std::string base = this->dirDataCache + "/ucernvm-";
    if (filename.substr(0,base.length()).compare(base) != 0) return ""; // Invalid
    return filename.substr(base.length(), filename.length()-base.length()-4); // Strip extension
};


/**
 * Check if the given CernVM version is cached
 */
int Hypervisor::cernVMCached( std::string version, std::string * filename ) {
    string sOutput = this->dirDataCache + "/ucernvm-" + version + ".iso";
    if (file_exists(sOutput)) {
        *filename = sOutput;
    } else {
        *filename = "";
    }
    return 0;
}

/**
 * Download the specified CernVM version
 */
int Hypervisor::cernVMDownload( std::string version, std::string * filename, HVPROGRESS_FEEDBACK * fb ) {
    string sURL = "http://cernvm.cern.ch/portal/sites/cernvm.cern.ch/files/ucernvm-" + version + ".iso";
    string sOutput = this->dirDataCache + "/ucernvm-" + version + ".iso";
    *filename = sOutput;
    if (file_exists(sOutput)) {
        return 0;
    } else {
        return downloadFile(sURL, sOutput, fb);
    }
};

/**
 * Cross-platform exec and return for the hypervisor control binary
 */
int Hypervisor::exec( string args, vector<string> * stdoutList ) {
    
    /* Build cmdline */
    string cmdline( this->hvBinary );
    cmdline += " " + args;
    
    /* Execute */
    return sysExec( cmdline, stdoutList );

}

/**
 * Initialize hypervisor 
 */
Hypervisor::Hypervisor() {
    this->sessionID = 1;
    
    /* Pick a system folder to store persistent information  */
    this->dirData = getAppDataPath();
    this->dirDataCache = this->dirData + "/cache";
    
};

/**
 * Exec version and parse version
 */
void Hypervisor::detectVersion() {
    vector<string> out;
    if (this->type = HV_VIRTUALBOX) {
        this->exec("--version", &out);
        
    } else {
        this->verString = "Unknown";
        this->verMajor = 0;
        this->verMinor = 0;
        return;
    }
    
    /* Get version string */
    string ver = out[0];
    unsigned nl = ver.find_first_of("\r\n");
    this->verString = ver.substr(0, nl);
    
    /* Get major */
    unsigned mav = ver.find(".");
    this->verMajor = ston<int>( ver.substr(0,mav) );
    
    /* Get minor */
    unsigned miv = ver.find(".", mav+1);
    this->verMinor = ston<int>( ver.substr(mav+1,miv-mav) );
    
};

/**
 * Allocate a new hypervisor session
 */
HVSession * Hypervisor::allocateSession ( std::string name, std::string key ) {
    HVSession * sess = new HVSession();
    sess->name = name;
    sess->key = key;
    sess->daemonControlled = false;
    sess->daemonMinCap = 0;
    sess->daemonMaxCap = 100;
    return sess;
}

/**
 * Release the memory used by a hypervisor session
 */
int Hypervisor::freeSession ( HVSession * session ) {
    delete session;
    return 0;
}

/**
 * Check the status of the session. It returns the following values:
 *  0 - Does not exist
 *  1 - Exist and has a valid key
 *  2 - Exists and has an invalid key
 */
int Hypervisor::sessionValidate ( std::string name, std::string key ) {
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->name.compare(name) == 0) {
            if (sess->key.compare(key) == 0) { /* Check secret key */
                return 1;
            } else {
                return 2;
            }
        }
    }
    return 0;
}

/**
 * Get a session using it's unique ID
 */
HVSession * Hypervisor::sessionLocate( std::string uuid ) {
    
    /* Check for running sessions with the given uuid */
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->uuid.compare(uuid) == 0) {
            return sess;
        }
    }
    
    /* Not found */
    return NULL;
    
}

/**
 * Open or reuse a hypervisor session
 */
HVSession * Hypervisor::sessionOpen( std::string name, std::string key ) { 
    
    /* Check for running sessions with the given credentials */
    std::cout << "Checking sessions (" << this->sessions.size() << ")\n";
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        std::cout << "Checking session name=" << sess->name << ", key=" << sess->key << ", uuid=" << sess->uuid << ", state=" << sess->state << "\n";
        
        if (sess->name.compare(name) == 0) {
            if (sess->key.compare(key) == 0) { /* Check secret key */
                return sess;
            } else {
                return NULL;
            }
        }
    }
    
    /* Allocate and register a new session */
    HVSession* sess = this->allocateSession( name, key );
    if (sess == NULL) return NULL;
    this->registerSession( sess );
    
    /* Return the handler */
    return sess;
    
}

/**
 * Register a session to the session stack and allocate a new unique ID
 */
int Hypervisor::registerSession( HVSession * sess ) {
    sess->internalID = this->sessionID++;
    this->sessions.push_back(sess);
    std::cout << "Updated sessions (" << this->sessions.size() << ")\n";
    return 0;
}

/**
 * Release a session using the given id
 */
int Hypervisor::sessionFree( int id ) {
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->internalID == id) {
            this->sessions.erase(i);
            delete sess;
            return 0;
        }
    }
    return HVE_NOT_FOUND;
}

/**
 * Return session for the given ID
 */
HVSession * Hypervisor::sessionGet( int id ) {
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->internalID == id) return sess;
    }
    return NULL;
}

/* Check if we need to start or stop the daemon */
int Hypervisor::checkDaemonNeed() {
    
    // If we haven't specified where the daemon is, we cannot do much
    if (daemonBinPath.empty()) return HVE_NOT_SUPPORTED;
    
    // Check if at least one session uses daemon
    bool daemonNeeded = false;
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->daemonControlled) {
            daemonNeeded = true;
            break;
        }
    }
    
    // Check if the daemon state is valid
    bool daemonState = isDaemonRunning(getDaemonLockfile());
    if (daemonNeeded != daemonState) {
        if (daemonNeeded) {
            return daemonStart( daemonBinPath ); /* START the daemon */
        } else {
            return daemonGet( DIPC_SHUTDOWN ); /* KILL the daemon */
        }
    }
    
    // No change
    return HVE_OK;
    
}

/**
 * Search the system's folders and try to detect what hypervisor
 * the user has installed and it will then populate the Hypervisor Config
 * structure that was passed to it.
 */
Hypervisor * detectHypervisor( std::string pathToDaemonBin ) {
    using namespace std;
    
    Hypervisor * hv;
    vector<string> paths;
    string bin, p;
    
    /* In which directories to look for the binary */
    #ifdef _WIN32
    paths.push_back( "C:/Program Files/Oracle/VirtualBox" );
    paths.push_back( "C:/Program Files (x86)/Oracle/VirtualBox" );
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    paths.push_back( "/Applications/VirtualBox.app/Contents/MacOS" );
    #endif
    #ifdef __linux__
    paths.push_back( "/usr" );
    paths.push_back( "/usr/local" );
    paths.push_back( "/opt/VirtualBox" );
    #endif
    
    /* Detect hypervisor */
    for (vector<string>::iterator i = paths.begin(); i != paths.end(); i++) {
        p = *i;
        
        /* Check for virtualbox */
        #ifdef _WIN32
        bin = p + "/VBoxManage.exe";
        if (file_exists(bin)) {
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p;
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            bin = p + "/VBoxGuestAdditions.iso";
            if (file_exists(bin)) {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->loadSessions();
            hv->daemonBinPath = pathToDaemonBin;
            return hv;
        }
        #endif
        #if defined(__APPLE__) && defined(__MACH__)
        bin = p + "/VBoxManage";
        if (file_exists(bin)) {
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p;
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            bin = p + "/VBoxGuestAdditions.iso";
            if (file_exists(bin)) {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->loadSessions();
            hv->daemonBinPath = pathToDaemonBin;
            return hv;
        }
        #endif
        #ifdef __linux__
        bin = p + "/bin/VBoxManage";
        if (file_exists(bin)) {
            hv = new Virtualbox();
            hv->type = HV_VIRTUALBOX;
            hv->hvRoot = p + "/bin";
            hv->hvBinary = bin;
            ((Virtualbox*)hv)->hvGuestAdditions = "";
            bin = p + "/VBoxGuestAdditions.iso";
            if (file_exists(bin)) {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->loadSessions();
            hv->daemonBinPath = pathToDaemonBin;
            return hv;
        }
        #endif
        
    }
    
    /* No hypervisor found */
    return NULL;
}

/**
 * Free the pointer(s) allocated by the detectHypervisor()
 */
void freeHypervisor( Hypervisor * hv ) {
    delete hv;
};

/**
 * Install hypervisor
 */
int installHypervisor( string versionID, void(*cbProgress)(int, int, std::string, void*), void * cbData ) {
    
    /**
     * Contact the information point
     */
    string requestBuf;
    cout << "INFO: Fetching data\n";
    if (cbProgress!=NULL) (cbProgress)(1, 100, "Checking the appropriate hypervisor for your system", cbData);
    int res = downloadText( "http://labs.wavesoft.gr/lhcah/?vid=" + versionID, &requestBuf );
    if ( res != HVE_OK ) return res;
    
    /**
     * Extract information
     */
    vector<string> lines;
    splitLines( requestBuf, &lines );
    map<string, string> data = tokenize( &lines, '=' );
    
    /**
     * Pick the URLs to download from
     */
    #ifdef _WIN32
    const string kDownloadUrl = "win32";
    const string kChecksum = "win32-sha256";
    const string kInstallerName = "win32-installer";
	const string kFileExt = ".exe";
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    const string kDownloadUrl = "osx";
    const string kChecksum = "osx-sha256";
    const string kInstallerName = "osx-installer";
	const string kFileExt = ".dmg";
    #endif
    #ifdef __linux__
    
    // Do some more in-depth analysis of the linux platform
    LINUX_INFO linuxInfo;
    getLinuxInfo( &linuxInfo );

    // Detect
    #if defined(__LP64__) || defined(_LP64)
    string kDownloadUrl = "linux64-" + linuxInfo.osDistID;
    #else
    string kDownloadUrl = "linux32-" + linuxInfo.osDistID;
    #endif
    
    // Calculate keys for other installers
    string kChecksum = kDownloadUrl + "-sha256";
    string kInstallerName = kDownloadUrl + "-installer";
	
    std::cout << "Download URL key = '" << kDownloadUrl << "'" << std::endl;
    std::cout << "Checksum key = '" << kChecksum << "'" << std::endl;
    std::cout << "Installer key = '" << kInstallerName << "'" << std::endl;
	
    #endif
    
    /**
     * Verify that the keys we are looking for exist
     */
    if (data.find( kDownloadUrl ) == data.end()) {
        cout << "ERROR: No download URL data found\n";
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kChecksum ) == data.end()) {
        cout << "ERROR: No checksum data found\n";
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kInstallerName ) == data.end()) {
        cout << "ERROR: No installer program data found\n";
        return HVE_EXTERNAL_ERROR;
    }
    
    #ifdef __linux__
    // Pick an extension and installation type based on the installer= parameter
    int installerType = PMAN_NONE;
	string kFileExt = ".run";
    if (data[kInstallerName].compare("dpkg") == 0) {
        installerType = PMAN_DPKG; /* Using debian installer */
    	kFileExt = ".deb";
    } else if (data[kInstallerName].compare("yum") == 0) {
        installerType = PMAN_YUM; /* Using 'yum localinstall <package> -y' */
    	kFileExt = ".rpm";
    }
    #endif
    
    /**
     * Prepare download feedback
     */
    HVPROGRESS_FEEDBACK feedback;
    feedback.total = 100;
    feedback.min = 2;
    feedback.max = 90;
    feedback.callback = cbProgress;
    feedback.data = cbData;
    feedback.message = "Downloading hypervisor";

    /**
     * Download
     */
    string tmpHypervisorInstall = getTmpFile( kFileExt );
    if (cbProgress!=NULL) (cbProgress)(2, 100, "Downloading hypervisor", cbData);
    cout << "INFO: Downloading " << data[kDownloadUrl] << " to " << tmpHypervisorInstall << "\n";
    res = downloadFile( data[kDownloadUrl], tmpHypervisorInstall, &feedback );
    cout << "    : Got " << res << "\n";
    if ( res != HVE_OK ) return res;
    
    /**
     * Validate checksum
     */
    string checksum;
    sha256_file( tmpHypervisorInstall, &checksum );
    if (cbProgress!=NULL) (cbProgress)(90, 100, "Validating download", cbData);
    cout << "INFO: File checksum " << checksum << " <-> " << data[kChecksum] << "\n";
    if (checksum.compare( data[kChecksum] ) != 0) return HVE_NOT_VALIDATED;
    
    /**
     * OS-Dependant installation process
     */
    #if defined(__APPLE__) && defined(__MACH__)

		cout << "INFO: Attaching\n";
		if (cbProgress!=NULL) (cbProgress)(94, 100, "Mounting hypervisor DMG disk", cbData);
		res = sysExec("hdiutil attach " + tmpHypervisorInstall, &lines);
		if (res != 0) {
			remove( tmpHypervisorInstall.c_str() );
			return HVE_EXTERNAL_ERROR;
		}
		string infoLine = lines.back();
		string dskDev, dskVolume, extra;
		getKV( infoLine, &dskDev, &extra, ' ', 0);
		getKV( extra, &extra, &dskVolume, ' ', dskDev.size()+1);
		cout << "Got disk '" << dskDev << "', volume: '" << dskVolume << "'\n";
    
		if (cbProgress!=NULL) (cbProgress)(97, 100, "Starting installer", cbData);
		cout << "INFO: Installing using " << dskVolume << "/" << data[kInstallerName] << "\n";
		res = sysExec("open -W " + dskVolume + "/" + data[kInstallerName], NULL);
		if (res != 0) {
			cout << "INFO: Detaching\n";
			res = sysExec("hdiutil detach " + dskDev, NULL);
			remove( tmpHypervisorInstall.c_str() );
			return HVE_EXTERNAL_ERROR;
		}
		cout << "INFO: Detaching\n";
		if (cbProgress!=NULL) (cbProgress)(100, 100, "Cleaning-up", cbData);
		res = sysExec("hdiutil detach " + dskDev, NULL);
		remove( tmpHypervisorInstall.c_str() );

	#elif defined(_WIN32)

		/* Start installer */
		if (cbProgress!=NULL) (cbProgress)(97, 100, "Starting installer", cbData);
		cout << "INFO: Starting installer\n";

		/* CreateProcess does not work because we need elevated permissions,
		 * use the classic ShellExecute to run the installer... */
		SHELLEXECUTEINFOA shExecInfo = {0};
		shExecInfo.cbSize = sizeof( SHELLEXECUTEINFO );
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = NULL;
		shExecInfo.lpVerb = NULL;
		shExecInfo.lpFile = (LPCSTR)tmpHypervisorInstall.c_str();
		shExecInfo.lpParameters = (LPCSTR)"";
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOW;
		shExecInfo.hInstApp = NULL;

		/* Validate handle */
		if ( !ShellExecuteExA( &shExecInfo ) ) {
			cout << "ERROR: Installation could not start! Error = " << res << endl;
			remove( tmpHypervisorInstall.c_str() );
			return HVE_EXTERNAL_ERROR;
		}

		/* Wait for termination */
		WaitForSingleObject( shExecInfo.hProcess, INFINITE );

		/* Cleanup */
		if (cbProgress!=NULL) (cbProgress)(100, 100, "Cleaning-up", cbData);
		remove( tmpHypervisorInstall.c_str() );

	#elif defined(__linux__)

        /* Check if our environment has what the installer needs */
		if (cbProgress!=NULL) (cbProgress)(92, 100, "Validating OS environment", cbData);
        if ((installerType != PMAN_NONE) && (installerType != linuxInfo.osPackageManager )) {
            cout << "ERROR: OS does not have the required package manager (type=" << installerType << ")" << endl;
			remove( tmpHypervisorInstall.c_str() );
            return HVE_NOT_FOUND;
        }

        /* (1) If we have xdg-open, use it to prompt the user using the system's default GUI */
		if (cbProgress!=NULL) (cbProgress)(94, 100, "Prompting user for the installation process", cbData);
        if (linuxInfo.hasXDGOpen) {
            string cmdline = "/usr/bin/xdg-open \"" + tmpHypervisorInstall + "\"";
            res = system( cmdline.c_str() );
    		if (res < 0) {
    			cout << "ERROR: Could not start. Return code: " << res << endl;
    			remove( tmpHypervisorInstall.c_str() );
    			return HVE_EXTERNAL_ERROR;
    		}
        
            /* TODO: Currently we don't do it, but we must wait until the package is installed
             *       and then do hypervisor probing again. */
            return HVE_SCHEDULED;
        
        /* (2) If we have GKSudo, do directly dpkg/yum install */
        } else if (linuxInfo.hasGKSudo) {
            string cmdline = "/bin/sh '" + tmpHypervisorInstall + "'";
            if ( installerType == PMAN_YUM ) {
                cmdline = "/usr/bin/yum localinstall '" + tmpHypervisorInstall + "' -y";
            } else if ( installerType == PMAN_DPKG ) {
                cmdline = "/usr/bin/dpkg -i '" + tmpHypervisorInstall + "'";
            }
            
            /* Use GKSudo to invoke the cmdline */
    		cmdline = "/usr/bin/gksudo \"" + cmdline + "\"";
    		res = system( cmdline.c_str() );
    		if (res < 0) {
    			cout << "ERROR: Could not start. Return code: " << res << endl;
    			remove( tmpHypervisorInstall.c_str() );
    			return HVE_EXTERNAL_ERROR;
    		}
        
        /* (3) Otherwise create a bash script and prompt the user */
        } else {
            
            /* TODO: I can't do much here :( */
            return HVE_NOT_IMPLEMENTED;
            
        }
        
    #endif
    
    /**
     * Check if it was successful
     */
    Hypervisor * hv = detectHypervisor( "" );
    if (hv == NULL) {
        cout << "ERROR: Could not install hypervisor!\n";
        return HVE_NOT_VALIDATED;
    };
    
    /**
     * Completed
     */
    return HVE_OK;
    
}
