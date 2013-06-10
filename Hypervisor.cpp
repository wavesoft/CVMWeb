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
#include "floppyIO.h"

using namespace std;

/* Incomplete type placeholders */
int Hypervisor::loadSessions()                                      { return HVE_NOT_IMPLEMENTED; }
int Hypervisor::updateSession( HVSession * session )                { return HVE_NOT_IMPLEMENTED; }
int Hypervisor::getCapabilities ( HVINFO_CAPS * )                   { return HVE_NOT_IMPLEMENTED; }
int HVSession::pause()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::close()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::stop()                                               { return HVE_NOT_IMPLEMENTED; }
int HVSession::resume()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::hibernate()                                          { return HVE_NOT_IMPLEMENTED; }
int HVSession::reset()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::update()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::open( int cpus, int memory, int disk, std::string cvmVersion, int flags ) 
                                                                    { return HVE_NOT_IMPLEMENTED; }
int HVSession::start( std::string userData )                        { return HVE_NOT_IMPLEMENTED; }
int HVSession::setExecutionCap(int cap)                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::setProperty( std::string name, std::string key )     { return HVE_NOT_IMPLEMENTED; }
std::string HVSession::getIP()                                      { return ""; }
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
 * Use FloppyIO to create a configuration disk for this VM
 */
int Hypervisor::buildFloppyIO ( std::string userData, std::string * filename ) {
    ofstream isoFile;
    string floppy = getTmpFile(".img");
    
    /* Write data (don't wait for sync) */
    FloppyIO * fio = new FloppyIO( floppy.c_str() );
    fio->send( userData );
    delete fio;
    
    /* Store the filename */
    *filename = floppy;
    return HVE_OK;
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
 * Download the specified generic, compressed disk image
 */
int Hypervisor::diskImageDownload( std::string url, std::string checksum, std::string * filename, HVPROGRESS_FEEDBACK * fb ) {
    string sURL = url;
    int res;
    
    // Create a custom HVPROGRESS_FEEDBACK in order to 
    // use the higher part for the extracting.
    HVPROGRESS_FEEDBACK nfb;
    if (fb != NULL) {
        nfb.min = fb->min;
        nfb.max = fb->max - 1;
        nfb.total = fb->total;
        nfb.data = fb->data;
        nfb.message = fb->message;
        nfb.lastEventTime = fb->lastEventTime;
        nfb.callback = fb->callback;
    } else {
        nfb.callback = NULL;
    }
    
    // Calculate the SHA256 checksum of the URL
    string sChecksum = "";
    sha256_buffer( url, &sChecksum );
    
    // Use the checksum as index
    string sGZOutput = this->dirDataCache + "/disk-" + sChecksum + ".vdi.gz";
    string sOutput = this->dirDataCache + "/disk-" + sChecksum + ".vdi";

    // Check if we have the uncompressed image in place
    *filename = sOutput;
    if (file_exists(sOutput)) {
        return HVE_ALREADY_EXISTS;

    }
    
    // Try again if we failed/aborted the image decompression
    if (file_exists(sGZOutput)) {
        
        // Validate file integrity
        sha256_file( sGZOutput, &sChecksum );
        if (sChecksum.compare( checksum ) != 0) {
            
            // Invalid checksum, remove file
            remove( sGZOutput.c_str() );
            
            // (Let the next block re-download the file)
            
        } else {
            
            // Notify progress
            if ((fb != NULL) && (fb->callback != NULL)) fb->callback( fb->max, fb->max, "Extracting compressed disk", fb->data );
            res = decompressFile( sGZOutput, sOutput );
            if (res != HVE_OK) 
                return res;
        
            // Delete sGZOutput if sOutput is there
            if (file_exists(sOutput)) {
                remove( sGZOutput.c_str() );
            } else {
                return HVE_EXTERNAL_ERROR;
            }
        
            // We got the filename
            return HVE_OK;

        }
    }
    
    // (Nothing is there, download it now)
    
    // Download the file to sGZOutput
    res = downloadFile(sURL, sGZOutput, &nfb);
    if (res != HVE_OK) return res;
    
    // Decompress
    if ((fb != NULL) && (fb->callback != NULL)) fb->callback( fb->max, fb->max, "Extracting compressed disk", fb->data );
    res = decompressFile( sGZOutput, sOutput );
    if (res != HVE_OK) 
        return res;
    
    // Delete sGZOutput if sOutput is there
    if (file_exists(sOutput)) {
        remove( sGZOutput.c_str() );
    } else {
        return HVE_EXTERNAL_ERROR;
    }
    
    // We got the filename
    return HVE_OK;

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
    sess->daemonFlags = 0;
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
HVSession * Hypervisor::sessionOpen( const std::string & name, const std::string & key ) { 
    
    /* Check for running sessions with the given credentials */
    CVMWA_LOG( "Info", "Checking sessions (" << this->sessions.size() << ")");
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        CVMWA_LOG( "Info", "Checking session name=" << sess->name << ", key=" << sess->key << ", uuid=" << sess->uuid << ", state=" << sess->state  );
        
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
    CVMWA_LOG( "Info", "Updated sessions (" << this->sessions.size()  );
    return HVE_OK;
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
            return HVE_OK;
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
    
    CVMWA_LOG( "checkDaemonNeed", "Checking daemon needs" );
    
    // If we haven't specified where the daemon is, we cannot do much
    if (daemonBinPath.empty()) return HVE_NOT_SUPPORTED;
    
    // Check if at least one session uses daemon
    bool daemonNeeded = false;
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        CVMWA_LOG( "checkDaemonNeed", "Session " << sess->uuid << ", daemonControlled=" << sess->daemonControlled << ", state=" << sess->state );
        if ( sess->daemonControlled && ((sess->state == STATE_OPEN) || (sess->state == STATE_STARTED) || (sess->state == STATE_PAUSED)) ) {
            daemonNeeded = true;
            break;
        }
    }
    
    // Check if the daemon state is valid
    bool daemonState = isDaemonRunning();
    CVMWA_LOG( "checkDaemonNeed", "Daemon is " << daemonState << ", daemonNeed is " << daemonNeeded );
    if (daemonNeeded != daemonState) {
        if (daemonNeeded) {
            CVMWA_LOG( "checkDaemonNeed", "Starting daemon" );
            return daemonStart( daemonBinPath ); /* START the daemon */
        } else {
            CVMWA_LOG( "checkDaemonNeed", "Stopping daemon" );
            return daemonStop(); /* KILL the daemon */
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
Hypervisor * detectHypervisor() {
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
            hv->daemonBinPath = "";
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
            hv->daemonBinPath = "";
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
            
            // (1) Check for additions on XXX/share/virtualbox [/usr, /usr/local]
            bin = p + "/share/virtualbox/VBoxGuestAdditions.iso";
            if (!file_exists(bin)) {

                // (2) Check for additions on XXX/additions [/opt/VirtualBox]
                bin = p + "/additions/VBoxGuestAdditions.iso";
                if (file_exists(bin)) {
                    ((Virtualbox*)hv)->hvGuestAdditions = bin;
                }

            } else {
                ((Virtualbox*)hv)->hvGuestAdditions = bin;
            }
            hv->detectVersion();
            hv->loadSessions();
            hv->daemonBinPath = "";
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
    CVMWA_LOG( "Info", "Fetching data" );
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
	
    CVMWA_LOG( "Info", "Download URL key = '" << kDownloadUrl << "'"  );
    CVMWA_LOG( "Info", "Checksum key = '" << kChecksum << "'"  );
    CVMWA_LOG( "Info", "Installer key = '" << kInstallerName << "'"  );
	
    #endif
    
    /**
     * Verify that the keys we are looking for exist
     */
    if (data.find( kDownloadUrl ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No download URL data found" );
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kChecksum ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No checksum data found" );
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kInstallerName ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No installer program data found" );
        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( "extpack" ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No extensions package URL found" );
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
    CVMWA_LOG( "Info", "Downloading " << data[kDownloadUrl] << " to " << tmpHypervisorInstall  );
    res = downloadFile( data[kDownloadUrl], tmpHypervisorInstall, &feedback );
    CVMWA_LOG( "Info", "    : Got " << res  );
    if ( res != HVE_OK ) return res;
    
    /**
     * Validate checksum
     */
    string checksum;
    sha256_file( tmpHypervisorInstall, &checksum );
    if (cbProgress!=NULL) (cbProgress)(90, 100, "Validating download", cbData);
    CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data[kChecksum]  );
    if (checksum.compare( data[kChecksum] ) != 0) return HVE_NOT_VALIDATED;
    
    /**
     * OS-Dependant installation process
     */
    #if defined(__APPLE__) && defined(__MACH__)

		CVMWA_LOG( "Info", "Attaching" );
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
		CVMWA_LOG( "Info", "Got disk '" << dskDev << "', volume: '" << dskVolume  );
    
		if (cbProgress!=NULL) (cbProgress)(97, 100, "Starting installer", cbData);
		CVMWA_LOG( "Info", "Installing using " << dskVolume << "/" << data[kInstallerName]  );
		res = sysExec("open -W " + dskVolume + "/" + data[kInstallerName], NULL);
		if (res != 0) {
			CVMWA_LOG( "Info", "Detaching" );
			res = sysExec("hdiutil detach " + dskDev, NULL);
			remove( tmpHypervisorInstall.c_str() );
			return HVE_EXTERNAL_ERROR;
		}
		CVMWA_LOG( "Info", "Detaching" );
		if (cbProgress!=NULL) (cbProgress)(100, 100, "Cleaning-up", cbData);
		res = sysExec("hdiutil detach " + dskDev, NULL);
		remove( tmpHypervisorInstall.c_str() );

	#elif defined(_WIN32)

		/* Start installer */
		if (cbProgress!=NULL) (cbProgress)(97, 100, "Starting installer", cbData);
		CVMWA_LOG( "Info", "Starting installer" );

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
		if (cbProgress!=NULL) (cbProgress)(94, 100, "Installing hypervisor", cbData);
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
     		if (cbProgress!=NULL) (cbProgress)(100, 100, "Passing control to the user", cbData);
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

            /* Cleanup */
    		if (cbProgress!=NULL) (cbProgress)(100, 100, "Cleaning-up", cbData);
        	remove( tmpHypervisorInstall.c_str() );
    	
        /* (3) Otherwise create a bash script and prompt the user */
        } else {
            
            /* TODO: I can't do much here :( */
            return HVE_NOT_IMPLEMENTED;
            
        }
        
    #endif
    
    /**
     * Check if it was successful
     */
    Hypervisor * hv = detectHypervisor();
    if (hv == NULL) {
        CVMWA_LOG( "Info", "ERROR: Could not install hypervisor!" );
        return HVE_NOT_VALIDATED;
    };
    
    /**
     * Completed
     */
    return HVE_OK;
    
}
