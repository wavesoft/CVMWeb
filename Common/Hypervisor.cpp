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

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include "CVMGlobals.h"
#include "Utilities.h"
#include "Hypervisor.h"
#include "DaemonCtl.h"

#include "contextiso.h"
#include "floppyIO.h"

#include "Hypervisor/Virtualbox/VBoxHypervisor.h"

using namespace std;
namespace fs = boost::filesystem;

/////////////////////////////////////
/////////////////////////////////////
////
//// Automatic version class
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Prepare regular expression for version parsing
 *
 * Identifies version strings in the following format:
 *
 *  <major>.<minor>[.revision>][-other]
 *
 */
boost::regex reVersionParse("(\\d+)\\.(\\d+)(?:\\.(\\d+))?(?:[\\.\\-\\w](\\d+))?(?:[\\.\\-\\w]([\\w\\.\\-]+))?"); 

/**
 * Constructor of version class
 */
HypervisorVersion::HypervisorVersion( const std::string& verString ) {

    // Reset values
    this->major = 0;
    this->minor = 0;
    this->build = 0;
    this->revision = 0;
    this->misc = "";
    this->verString = "";

    // Try to match the expression
    boost::smatch matches;
    if (boost::regex_match(verString, matches, reVersionParse, boost::match_extra)) {
        if (matches.size() > 1) {

            // Get the entire matched string
            string stringMatch(matches[1].first, matches[1].second);
            this->verString = stringMatch;

            // 

        }

        // matches[0] contains the original string.  matches[n]
        // contains a sub_match object for each matching
        // subexpression

         for (int i = 1; i < matches.size(); i++)
         {
            // sub_match::first and sub_match::second are iterators that
            // refer to the first and one past the last chars of the
            // matching subexpression
            string match(matches[i].first, matches[i].second);
            cout << "\tmatches[" << i << "] = " << match << endl;
         }
      }
}


/////////////////////////////////////
/////////////////////////////////////
////
//// Tool Functions
////
/////////////////////////////////////
/////////////////////////////////////

/** 
 * Return the string representation of the given error code
 */
std::string hypervisorErrorStr( int error ) {
    CRASH_REPORT_BEGIN;
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
    CRASH_REPORT_END;
};

/**
 * Utility function to forward decompression event from another thread
 */
void __notifyDecompressStart( const VariableTaskPtr & pf ) {
    pf->doing("Extracting compressed disk");
}

/**
 * Decompress phase
 */
int __diskExtract( const std::string& sGZOutput, const std::string& checksum, const std::string& sOutput, const VariableTaskPtr & pf ) {
    CRASH_REPORT_BEGIN;
    std::string sChecksum;
    int res;
    
    // Validate file integrity
    sha256_file( sGZOutput, &sChecksum );
    if (sChecksum.compare( checksum ) != 0) {
        
        // Invalid checksum, remove file
        CVMWA_LOG("Info", "Invalid local checksum (" << sChecksum << ")");
        ::remove( sGZOutput.c_str() );
        
        // Mark the progress as completed (this is not a failure. We are going to retry)
        if (pf) pf->complete("Going to re-download");

        // (Let the next block re-download the file)
        return HVE_NOT_VALIDATED;
        
    } else {
        
        // Notify progress (from another thread, because we are going to be blocking soon)
        boost::thread * t = NULL;
        if (pf) {
            pf->setMax(1);
            t = new boost::thread( boost::bind( &__notifyDecompressStart, pf ) );
        }

        // Decompress the file
        CVMWA_LOG("Info", "File exists and checksum valid, decompressing " << sGZOutput << " to " << sOutput );
        res = decompressFile( sGZOutput, sOutput );
        if (res != HVE_OK) {

            // Notify error and reap notification thrad
            if (t != NULL) { 
                pf->fail( "Unable to decompress the disk image", res );
                t->join(); 
                delete t; 
            }

            // Return error code
            return res;

        }
    
        // Delete sGZOutput if sOutput is there
        if (file_exists(sOutput)) {
            CVMWA_LOG("Info", "File is in place. Removing original" );
            ::remove( sGZOutput.c_str() );

        } else {
            CVMWA_LOG("Info", "Could not find the extracted file!" );
            if (t != NULL) { 
                pf->fail( "Could not find the extracted file", res );
                t->join(); 
                delete t; 
            }
            return HVE_EXTERNAL_ERROR;
        }
    
        // We got the filename
        if (t != NULL) { t->join(); delete t; }
        if (pf) pf->complete("Disk extracted");        
        return HVE_OK;

    }
    CRASH_REPORT_END;
}

/////////////////////////////////////
/////////////////////////////////////
////
//// HVSession Implementation
////
/////////////////////////////////////
/////////////////////////////////////

/* Incomplete type placeholders */
int HVSession::pause()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::close( bool unmonitored )                            { return HVE_NOT_IMPLEMENTED; }
int HVSession::stop()                                               { return HVE_NOT_IMPLEMENTED; }
int HVSession::resume()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::hibernate()                                          { return HVE_NOT_IMPLEMENTED; }
int HVSession::reset()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::update()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::updateFast()                                         { return HVE_NOT_IMPLEMENTED; }
int HVSession::open( int cpus, int memory, int disk, std::string cvmVersion, int flags ) 
                                                                    { return HVE_NOT_IMPLEMENTED; }
int HVSession::start( std::map<std::string,std::string> *userData ) { return HVE_NOT_IMPLEMENTED; }
int HVSession::setExecutionCap(int cap)                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::setProperty( std::string name, std::string key )     { return HVE_NOT_IMPLEMENTED; }
int HVSession::getAPIPort()                                         { return 0; }
std::string HVSession::getAPIHost()                                 { return ""; }
std::string HVSession::getProperty( std::string name )              { return ""; }
std::string HVSession::getRDPHost()                                 { return ""; }
std::string HVSession::getExtraInfo( int extraInfo )                { return ""; }

/**
 * Try to connect to the API port and check if it succeeded
 */

bool HVSession::isAPIAlive( unsigned char handshake ) {
    CRASH_REPORT_BEGIN;
    std::string ip = this->getAPIHost();;
    if (ip.empty()) return false;
    return isPortOpen( ip.c_str(), this->getAPIPort(), handshake );
    CRASH_REPORT_END;
}

/////////////////////////////////////
/////////////////////////////////////
////
//// Hypervisor Implementation
////
/////////////////////////////////////
/////////////////////////////////////

/* Incomplete type placeholders */
bool Hypervisor::waitTillReady(const FiniteTaskPtr & pf)  { return false; }

/**
 * Measure the resources from the sessions
 */
int Hypervisor::getUsage( HVINFO_RES * resCount ) { 
    CRASH_REPORT_BEGIN;
    resCount->memory = 0;
    resCount->cpus = 0;
    resCount->disk = 0;
    for (std::map< std::string,HVSessionPtr >::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSessionPtr sess = (*i).second;
        resCount->memory += sess->parameters->getNum<int>( "memory" );
        resCount->cpus += sess->parameters->getNum<int>( "cpus" );
        resCount->disk += sess->parameters->getNum<int>( "disk" );
    }
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Use LibcontextISO to create a cd-rom for this VM
 */
int Hypervisor::buildContextISO ( std::string userData, std::string * filename ) {
    CRASH_REPORT_BEGIN;
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
    CRASH_REPORT_END;
};

/**
 * Use FloppyIO to create a configuration disk for this VM
 */
int Hypervisor::buildFloppyIO ( std::string userData, std::string * filename ) {
    CRASH_REPORT_BEGIN;
    ofstream isoFile;
    string floppy = getTmpFile(".img");
    
    /* Write data (don't wait for sync) */
    FloppyIO * fio = new FloppyIO( floppy.c_str() );
    fio->send( userData );
    delete fio;
    
    /* Store the filename */
    *filename = floppy;
    return HVE_OK;
    CRASH_REPORT_END;
};

/**
 * Extract the CernVM version from the filename specified
 */
std::string Hypervisor::cernVMVersion( std::string filename ) {
    CRASH_REPORT_BEGIN;
    std::string base = this->dirDataCache + "/ucernvm-";
    if (filename.substr(0,base.length()).compare(base) != 0) return ""; // Invalid
    return filename.substr(base.length(), filename.length()-base.length()-4); // Strip extension
    CRASH_REPORT_END;
};


/**
 * Check if the given CernVM version is cached
 * This function optionally updates the filename pointer specified
 */
int Hypervisor::cernVMCached( std::string version, std::string * filename ) {
    CRASH_REPORT_BEGIN;
    string sOutput = this->dirDataCache + "/ucernvm-" + version + ".iso";
    if (file_exists(sOutput)) {
        if (filename != NULL) *filename = sOutput;
        return 1;
    } else {
        if (filename != NULL) *filename = "";
        return 0;
    }
    CRASH_REPORT_END;
}

/**
 * Download the specified CernVM version
 */
int Hypervisor::cernVMDownload( std::string version, std::string * filename, const FiniteTaskPtr & pf, std::string flavor, std::string arch ) {
    CRASH_REPORT_BEGIN;
    string sURL = URL_CERNVM_RELEASES "/ucernvm-images." + version + ".cernvm." + arch + "/ucernvm-" + flavor + "." + version + ".cernvm." + arch + ".iso";
    string sOutput = this->dirDataCache + "/ucernvm-" + version + ".iso";

    string sChecksumURL = sURL + ".sha256";
    string sChecksumOutput = sOutput + ".sha256";

    // Create a variable task for the download process
    VariableTaskPtr downloadPf;
    if (pf) {
        downloadPf = pf->begin<VariableTask>( "Downloading CernVM" );
    }

    // Start downloading
    *filename = sOutput;
    if (file_exists(sOutput)) {
        if (downloadPf) {
            downloadPf->complete("File already exists");
        }
        return 0;
    } else {
        return downloadProvider->downloadFile(sURL, sOutput, downloadPf);
    }

    CRASH_REPORT_END;
};


/**
 * Download the specified generic, compressed disk image
 */
int Hypervisor::diskImageDownload( std::string url, std::string checksum, std::string * filename, const FiniteTaskPtr & pf ) {
    CRASH_REPORT_BEGIN;
    string sURL = url;
    int res;
    
    CVMWA_LOG("Info", "Downloading disk image from " << url);

    // Set maximum number of tasks to perform
    if (pf) {
        pf->doing("Downloading disk image");
        pf->setMax(2);
    }
    
    // Calculate the SHA256 checksum of the URL
    string sChecksum = "";
    sha256_buffer( url, &sChecksum );
    
    // Use the checksum as index
    string sGZOutput = this->dirDataCache + "/disk-" + sChecksum + ".vdi.gz";
    string sOutput = this->dirDataCache + "/disk-" + sChecksum + ".vdi";
    CVMWA_LOG("Info", "Target disk file " << sGZOutput);

    // Check if we have the uncompressed image in place
    *filename = sOutput;
    if (file_exists(sOutput) && !file_exists(sGZOutput)) {
        CVMWA_LOG("Info", "Uncompressed file already exists");
        if (pf) pf->complete("Uncompressed file already in place");
        return HVE_ALREADY_EXISTS;

    }
    
    // Try again if we failed/aborted the image decompression
    if (file_exists(sGZOutput)) {

        // Prepare the decompress sub-task if for some reason we already have the disk file in place
        VariableTaskPtr compressPf;
        if (pf) compressPf = pf->begin<VariableTask>("Extracting disk file");
        
        // Check decompressing file
        res = __diskExtract( sGZOutput, checksum, sOutput, compressPf );

        // If checksum is invalid, re-download the file.
        // Otherwise we are good to exit.
        if (res != HVE_NOT_VALIDATED) return res;

    }
    
    // Prepare the download sub-task
    VariableTaskPtr downloadPf;
    if (pf) downloadPf = pf->begin<VariableTask>("Downloading disk file");

    // Download the file to sGZOutput
    CVMWA_LOG("Info", "Performing download from '" << sURL << "' to '" << sGZOutput << "'" );
    res = downloadProvider->downloadFile(sURL, sGZOutput, downloadPf);
    if (res != HVE_OK) return res;
    
    // Prepare the decompress sub-task
    VariableTaskPtr compressPf;
    if (pf) compressPf = pf->begin<VariableTask>("Extracting disk file");

    // Validate & Decompress
    return __diskExtract( sGZOutput, checksum, sOutput, compressPf );
    CRASH_REPORT_END;
};

/**
 * Cross-platform exec and return for the hypervisor control binary
 */
int Hypervisor::exec( string args, vector<string> * stdoutList, string * stderrMsg, int retries, int timeout ) {
    CRASH_REPORT_BEGIN;
    int execRes = 0;

    /* If retries is negative, do not monitor the execution */
    if (retries < 0) {

        /* Execute asynchronously */
        execRes = sysExecAsync( this->hvBinary, args );

    } else {
    
        /* Execute */
        string execError;
        execRes = sysExec( this->hvBinary, args, stdoutList, &execError, retries, timeout );
        if (stderrMsg != NULL) *stderrMsg = execError;

        /* Store the last error occured */
        if (!execError.empty())
            this->lastExecError = execError;

    }

    return execRes;
    CRASH_REPORT_END;
}

/**
 * Initialize hypervisor 
 */
Hypervisor::Hypervisor() {
    CRASH_REPORT_BEGIN;
    this->sessionID = 1;
    
    /* Pick a system folder to store persistent information  */
    this->dirData = getAppDataPath();
    this->dirDataCache = this->dirData + "/cache";
    
    /* Unless overriden use the default downloadProvider */
    this->downloadProvider = DownloadProvider::Default();
    
    /* Reset vars */
    this->verMajor = 0;
    this->verMinor = 0;
    this->type = 0;

    CRASH_REPORT_END;
};

/**
 * Exec version and parse version
 */
void Hypervisor::detectVersion() {
    CRASH_REPORT_BEGIN;
    vector<string> out;
    string err;
    if (this->type == HV_VIRTUALBOX) {
        this->exec("--version", &out, &err);
        
    } else {
        this->verString = "Unknown";
        this->verMajor = 0;
        this->verMinor = 0;
        return;
    }
    
    /* We don't have enough information */
    if (out.size() == 0) return;

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
    
    CRASH_REPORT_END;
};

/**
 * Check the status of the session. It returns the following values:
 *  0  - Does not exist
 *  1  - Exist and has a valid key
 *  2  - Exists and has an invalid key
 *  <0 - An error occured
 */
int Hypervisor::sessionValidate ( const ParameterMapPtr& parameters ) {
    CRASH_REPORT_BEGIN;

    // Extract name and key
    std::string name = parameters->get("name");
    if (name.empty()) {
        CVMWA_LOG("Error", "Missing 'name' parameter on sessionOpen" );
        return HVE_NOT_VALIDATED;
    }
    std::string key = parameters->get("key");
    if (name.empty()) {
        CVMWA_LOG("Error", "Missing 'key' parameter on sessionOpen" );
        return HVE_NOT_VALIDATED;
    }

    // Calculate the SHA256 checksum of the key, salted with a pre-defined salt
    std::string keyHash;
    sha256_buffer( CRYPTO_SALT + key, &keyHash );

    // Get a session that matches the given name
    HVSessionPtr sess = this->sessionByName( name );

    // No session found? Return 0
    if (!sess) return 0;

    // Compare key
    if (sess->parameters->get("key", "").compare(keyHash) == 0) {

        // Correct password
        return 1;

    } else {

        // Invalid password
        return 2;

    }

    CRASH_REPORT_END;
}

/**
 * Get a session using it's unique ID
 */
/*
HVSession * Hypervisor::sessionLocate( std::string uuid ) {
    CRASH_REPORT_BEGIN;
    
    // Check for running sessions with the given uuid
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->uuid.compare(uuid) == 0) {
            return sess;
        }
    }
    
    // Not found
    return NULL;
    
    CRASH_REPORT_END;
}
*/

/**
 * Open or reuse a hypervisor session
 */
HVSessionPtr Hypervisor::sessionOpen( const ParameterMapPtr& parameters ) { 
    CRASH_REPORT_BEGIN;
    
    // Default unsetted pointer used for invalid responses
    HVSessionPtr voidPtr;

    // Extract name and key
    std::string name = parameters->get("name");
    if (name.empty()) {
        CVMWA_LOG("Error", "Missing 'name' parameter on sessionOpen" );
        return voidPtr;
    }
    std::string key = parameters->get("key");
    if (name.empty()) {
        CVMWA_LOG("Error", "Missing 'key' parameter on sessionOpen" );
        return voidPtr;
    }

    // Calculate the SHA256 checksum of the key, salted with a pre-defined salt
    std::string keyHash;
    sha256_buffer( CRYPTO_SALT + key, &keyHash );

    // Get a session that matches the given name
    HVSessionPtr sess = this->sessionByName( name );
    
    // If we found one, continue
    if (sess) {
        // Validate secret key
        if (sess->parameters->get("key","").compare(keyHash) == 0) {

            // Exists and it's valid. Update parameters
            sess->parameters->fromParameters( parameters );

            // And return instance
            return sess;

        } else {
            // Exists but the password is invalid
            return voidPtr;
        }
    }

    // Otherwise, allocate one
    sess = this->allocateSession();
    if (!sess) return voidPtr;

    // Populate parameters
    sess->parameters->fromParameters( parameters );
    
    // Return the handler
    return sess;
    CRASH_REPORT_END;
}

/**
 * Return a session object by locating it by name
 */
HVSessionPtr Hypervisor::sessionByName ( const std::string& name ) {
    CRASH_REPORT_BEGIN;
    HVSessionPtr voidPtr;

    // Iterate over sessions
    for (std::map< std::string,HVSessionPtr >::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSessionPtr sess = (*i).second;

        // Session found
        if (sess->parameters->get("name","").compare(name) == 0) {
            return sess;
        }
    }

    // Return void
    return voidPtr;

    CRASH_REPORT_END;
}


/**
 * Register a session to the session stack and allocate a new unique ID
 */
/*
int Hypervisor::registerSession( HVSession * sess ) {
    CRASH_REPORT_BEGIN;
    sess->internalID = this->sessionID++;
    this->sessions.push_back(sess);
    CVMWA_LOG( "Info", "Updated sessions (" << this->sessions.size()  );
    return HVE_OK;
    CRASH_REPORT_END;
}
*/

/**
 * Release a session using the given id
 */
/*
int Hypervisor::sessionFree( int id ) {
    CRASH_REPORT_BEGIN;
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->internalID == id) {
            this->sessions.erase(i);
            delete sess;
            return HVE_OK;
        }
    }
    return HVE_NOT_FOUND;
    CRASH_REPORT_END;
}
*/

/**
 * Return session for the given ID
 */
/*
HVSession * Hypervisor::sessionGet( int id ) {
    CRASH_REPORT_BEGIN;
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->internalID == id) return sess;
    }
    return NULL;
    CRASH_REPORT_END;
}
*/

/* Check if we need to start or stop the daemon */
int Hypervisor::checkDaemonNeed() {
    CRASH_REPORT_BEGIN;
    
    CVMWA_LOG( "Info", "Checking daemon needs" );
    
    // If we haven't specified where the daemon is, we cannot do much
    if (daemonBinPath.empty()) {
        CVMWA_LOG( "Warning", "Daemon binary was not specified or misssing" );
        return HVE_NOT_SUPPORTED;
    }
    
    // Check if at least one session uses daemon
    bool daemonNeeded = false;
    for (std::map< std::string,HVSessionPtr >::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSessionPtr sess = (*i).second;
        int daemonControlled = sess->parameters->getNum<int>("daemonControlled");
        CVMWA_LOG( "Info", "Session " << sess->uuid << ", daemonControlled=" << daemonControlled << ", state=" << sess->state );
        if ( daemonControlled && ((sess->state == STATE_OPEN) || (sess->state == STATE_STARTED) || (sess->state == STATE_PAUSED)) ) {
            daemonNeeded = true;
            break;
        }
    }
    
    // Check if the daemon state is valid
    bool daemonState = isDaemonRunning();
    CVMWA_LOG( "Info", "Daemon is " << daemonState << ", daemonNeed is " << daemonNeeded );
    if (daemonNeeded != daemonState) {
        if (daemonNeeded) {
            CVMWA_LOG( "Info", "Starting daemon" );
            return daemonStart( daemonBinPath ); /* START the daemon */
        } else {
            CVMWA_LOG( "Info", "Stopping daemon" );
            return daemonStop(); /* KILL the daemon */
        }
    }
    
    // No change
    return HVE_OK;
    
    CRASH_REPORT_END;
}

/**
 * Change the default download provider
 */
void Hypervisor::setDownloadProvider( DownloadProviderPtr p ) { 
    CRASH_REPORT_BEGIN;
    this->downloadProvider = p;
    CRASH_REPORT_END;
};

/**
 * Search the system's folders and try to detect what hypervisor
 * the user has installed and it will then populate the Hypervisor Config
 * structure that was passed to it.
 */
Hypervisor * detectHypervisor() {
    CRASH_REPORT_BEGIN;
    Hypervisor * hv = NULL;

    /* 1) Look for Virtualbox */
    hv = vboxDetect();
    if (hv != NULL) return hv;

    /* 2) Check for other hypervisors */
    // TODO: Implement this
    
    /* No hypervisor found */
    return NULL;
    CRASH_REPORT_END;
}

/**
 * Free the pointer(s) allocated by the detectHypervisor()
 */
void freeHypervisor( Hypervisor * hv ) {
    CRASH_REPORT_BEGIN;
    delete hv;
    CRASH_REPORT_END;
};

/**
 * Install hypervisor
 */
int installHypervisor( string versionID, DownloadProviderPtr downloadProvider, const FiniteTaskPtr & pf, int retries ) {
    CRASH_REPORT_BEGIN;
    const int maxSteps = 200;
    Hypervisor * hv = NULL;
    int res;
    string tmpHypervisorInstall;
    string checksum;

    // Initialize progress feedback
    if (pf) {
        pf->setMax(3);
    }

    ////////////////////////////////////
    // Contact the information point
    ////////////////////////////////////
    string requestBuf;

    // Download trials
    for (int tries=0; tries<retries; tries++) {
        CVMWA_LOG( "Info", "Fetching data" );

        // Send progress feedback
        if (pf) pf->doing("Downloading hypervisor configuration");

        // Try to download the configuration URL
        res = downloadProvider->downloadText( URL_HYPERVISOR_CONFIG + versionID, &requestBuf );
        if ( res != HVE_OK ) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );

                // Send progress feedback
                if (pf) pf->doing("Re-downloading hypervisor configuration");

                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Too many retries while downloading hypervisor configuration");

            return res;

        } else {

            // Send progress feedback
            if (pf) pf->done("Downloaded hypervisor configuration");

            /* Reached this point, we are good to continue */
            break;            
        }

    }
    
    ////////////////////////////////////
    // Extract information
    ////////////////////////////////////

    // Prepare variables
    vector<string> lines;
    splitLines( requestBuf, &lines );
    map<string, string> data = tokenize( &lines, '=' );
    
    // Pick the URLs to download from
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
    
    ////////////////////////////////////
    // Verify information
    ////////////////////////////////////

    // Verify that the keys we are looking for exist
    if (data.find( kDownloadUrl ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No download URL data found" );

        // Send progress fedback
        if (pf) pf->fail("No hypervisor download URL data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kChecksum ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No checksum data found" );

        // Send progress fedback
        if (pf) pf->fail("No setup checksum data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (data.find( kInstallerName ) == data.end()) {
        CVMWA_LOG( "Error", "ERROR: No installer program data found" );

        // Send progress fedback
        if (pf) pf->fail("No installer program data found");

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

    ////////////////////////////////////
    // Download hypervisor installer
    ////////////////////////////////////

    // Prepare feedback pointers
    VariableTaskPtr downloadPf;
    if (pf) {
        downloadPf = pf->begin<VariableTask>("Downloading hypervisor installer");
    }

    // Download trials loop
    for (int tries=0; tries<retries; tries++) {

        // Download installer
        tmpHypervisorInstall = getTmpFile( kFileExt );
        CVMWA_LOG( "Info", "Downloading " << data[kDownloadUrl] << " to " << tmpHypervisorInstall  );
        res = downloadProvider->downloadFile( data[kDownloadUrl], tmpHypervisorInstall, downloadPf );
        CVMWA_LOG( "Info", "    : Got " << res  );
        if ( res != HVE_OK ) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->done("Unable to download hypervisor installer");
            return res;
        }
        
        // Validate checksum
        if (pf) pf->doing("Validating download");
        sha256_file( tmpHypervisorInstall, &checksum );

        CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data[kChecksum]  );
        if (checksum.compare( data[kChecksum] ) != 0) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Celeanup
            ::remove( tmpHypervisorInstall.c_str() );

            // Send progress fedback
            if (pf) pf->fail("Unable to validate hypervisor installer");
            return HVE_NOT_VALIDATED;
        }

        // Send progress feedback
        if (pf) pf->done("Hypervisor installer downloaded");

        // ( Reached this point, we are good to continue )
        break;

    }
    
    ////////////////////////////////////
    // OS-Dependant installation process
    ////////////////////////////////////
    
    // Prepare feedback pointers
    FiniteTaskPtr installerPf;
    if (pf) {
        installerPf = pf->begin<FiniteTask>("Installing hypervisor");
    }

    // Start installer with retries
    string errorMsg;
    for (int tries=0; tries<retries; tries++) {
        #if defined(__APPLE__) && defined(__MACH__)
            if (installerPf) installerPf->setMax(3, false);

    		CVMWA_LOG( "Info", "Attaching" << tmpHypervisorInstall );
            if (installerPf) installerPf->doing("Mouting hypervisor DMG disk");
    		res = sysExec("/usr/bin/hdiutil", "attach " + tmpHypervisorInstall, &lines, &errorMsg);
    		if (res != 0) {
                if (tries<retries) {
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
            		if (installerPf) installerPf->doing("Retrying installation");
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
    			::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to use hdiutil to mount DMG");

                return HVE_EXTERNAL_ERROR;
    		}
            if (installerPf) installerPf->done("Mounted DMG disk");

    		string infoLine = lines.back();
    		string dskDev, dskVolume, extra;
    		getKV( infoLine, &dskDev, &extra, ' ', 0);
    		getKV( extra, &extra, &dskVolume, ' ', dskDev.size()+1);
    		CVMWA_LOG( "Info", "Got disk '" << dskDev << "', volume: '" << dskVolume  );
    
    		if (installerPf) installerPf->doing("Starting installer");
    		CVMWA_LOG( "Info", "Installing using " << dskVolume << "/" << data[kInstallerName]  );
    		res = sysExec("/usr/bin/open", "-W " + dskVolume + "/" + data[kInstallerName], NULL, &errorMsg);
    		if (res != 0) {

                CVMWA_LOG( "Info", "Detaching" );
        		if (installerPf) installerPf->doing("Unmounting DMG");
    			res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg);
                if (tries<retries) {
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
            		if (installerPf) installerPf->doing("Restarting installer");
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
    			::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
    		}
            if (installerPf) installerPf->done("Installed hypervisor");

    		CVMWA_LOG( "Info", "Detaching" );
            if (installerPf) installerPf->doing("Cleaning-up");
    		res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg);
            if (installerPf) {
                installerPf->done("Cleaning-up completed");
                installerPf->complete("Installed hypervisor"):
            }

    	#elif defined(_WIN32)
            if (installerPf) installerPf->setMax(1, false);

    		// Start installer
            if (installerPf) installerPf->doing("Starting installer");
    		CVMWA_LOG( "Info", "Starting installer" );

    		// CreateProcess does not work because we need elevated permissions,
    		// use the classic ShellExecute to run the installer...
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

    		// Validate handle
    		if ( !ShellExecuteExA( &shExecInfo ) ) {
    			cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
    			::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
    		}

            // Validate hProcess
            if (shExecInfo.hProcess == 0) {
    			cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
    			::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }

    		// Wait for termination
    		WaitForSingleObject( shExecInfo.hProcess, INFINITE );
            if (installerPf) installerPf->done("Installer completed");

    		// Complete
            if (installerPf) installerPf->complete("Installed hypervisor");

    	#elif defined(__linux__)
            if (installerPf) installerPf->setMax(4, false);

            // Check if our environment has what the installer needs
            if (installerPf) installerPf->doing("Probing environment");
            if ((installerType != PMAN_NONE) && (installerType != linuxInfo.osPackageManager )) {
                cout << "ERROR: OS does not have the required package manager (type=" << installerType << ")" << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Re-probing environment");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
    			::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (pf) pf->fail("Unable to probe the environment");

                return HVE_NOT_FOUND;
            }
            if (installerPf) installerPf->done("Probed environment");

            // (1) If we have xdg-open, use it to prompt the user using the system's default GUI
            // ----------------------------------------------------------------------------------
            if (linuxInfo.hasXDGOpen) {

                if (installerPf) installerPf->doing("Starting hypervisor installer");
                string cmdline = "/usr/bin/xdg-open \"" + tmpHypervisorInstall + "\"";
                res = system( cmdline.c_str() );
        		if (res < 0) {
        			cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
    			    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to start the hypervisor installer");

        			return HVE_EXTERNAL_ERROR;
        		}
                if (installerPf) installerPf->done("Installer started");
            
                // At some point the process that xdg-open launches is
                // going to open the file in order to read it's contnets. 
                // Wait for 10 sec for it to happen
                if (installerPf) installerPf->doing("Waiting for the installation to begin");
                if (!waitFileOpen( tmpHypervisorInstall, true, 60000 )) { // 1 min until it's captured
        			cout << "ERROR: Could not wait for file handler capture: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Waiting again for the installation to begin");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
    			    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to check the status of the installation");
                    
                    return HVE_STILL_WORKING;
                }
                if (installerPf) installerPf->done("Installation started");

                // Wait for it to be released
                if (installerPf) installerPf->doing("Waiting for the installation to complete");
                if (!waitFileOpen( tmpHypervisorInstall, false, 900000 )) { // 15 mins until it's released
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Waiting again for the installation to complete");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // We can't remove the file, it's in use :(
                    CVMWA_LOG("Error", "ERROR: Could not wait for file handler release: " << res );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to check the status of the installation");

        			return HVE_STILL_WORKING;
                }

                // Done
                if (installerPf) installerPf->done("Installation completed");
        
    		    // Complete
                if (installerPf) installerPf->complete("Installed hypervisor");

            // (2) If we have GKSudo, do directly dpkg/yum install
            // ------------------------------------------------------
            } else if (linuxInfo.hasGKSudo) {
                string cmdline = "/bin/sh '" + tmpHypervisorInstall + "'";
                if ( installerType == PMAN_YUM ) {
                    cmdline = "/usr/bin/yum localinstall '" + tmpHypervisorInstall + "' -y";
                } else if ( installerType == PMAN_DPKG ) {
                    cmdline = "/usr/bin/dpkg -i '" + tmpHypervisorInstall + "'";
                }

                // Use GKSudo to invoke the cmdline
                if (installerPf) installerPf->doing("Starting installer");
        		cmdline = "/usr/bin/gksudo \"" + cmdline + "\"";
        		res = system( cmdline.c_str() );
        		if (res < 0) {
        			cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
    			    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (pf) pf->fail("Unable to start the hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
        		}
                if (installerPf) installerPf->done("Installer completed");

    		    // Complete
                if (installerPf) installerPf->complete("Installed hypervisor");
    	
            /* (3) Otherwise create a bash script and prompt the user */
            } else {
            
                /* TODO: I can't do much here :( */
                return HVE_NOT_IMPLEMENTED;
            
            }
        
        #endif
    
        // Give 5 seconds as a cool-down delay
        sleepMs(5000);

        // Check if it was successful
        hv = detectHypervisor();
        if (hv == NULL) {
            CVMWA_LOG( "Info", "ERROR: Could not install hypervisor!" );
            if (tries<retries) {
                if (installerPf) installerPf->restart("Re-trying hypervisor installation");
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Hypervisor installation seems not feasible");

            return HVE_NOT_VALIDATED;
        } else {

            /* Everything worked like a charm */
            ::remove( tmpHypervisorInstall.c_str() );
            break;

        }
        
    }

    /**
     * If we are installing VirtualBox, make sure the VBOX Extension pack are installed
     */
     /*
    if ((hv != NULL) && (hv->type == HV_VIRTUALBOX)) {
        if ( !((Virtualbox*)hv)->hasExtPack() ) {

            // Install extension pack, and fail on error
            if ( (res = ((Virtualbox*)hv)->installExtPack( versionID, downloadProvider, cbProgress, 110, maxSteps, maxSteps )) != HVE_OK ) {
                freeHypervisor(hv);
                return res;
            };

        }
    }
    */

    // Release hypervisor
    freeHypervisor(hv);

    // Completed
    if (pf) pf->complete("Hypervisor installed successfully");
    return HVE_OK;
    
    CRASH_REPORT_END;
}
