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

#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include "Config.h"
#include "Utilities.h"
#include "Hypervisor.h"
#include "DaemonCtl.h"

#include "contextiso.h"
#include "floppyIO.h"

#include "Hypervisor/Virtualbox/VBoxCommon.h"
#include "Hypervisor/Virtualbox/VBoxSession.h"

using namespace std;
namespace fs = boost::filesystem;

// The length of the checksum. For SHA256 that's 64
#define  CHECKSUM_LENGTH    64

/////////////////////////////////////
/////////////////////////////////////
////
//// Automatic version class
////
/////////////////////////////////////
/////////////////////////////////////

/**
 * Prepare regular expression for version parsing
 * Parses version strings in the following format:
 *
 *  <major>.<minor>[.revision>][-other]
 *
 */
boost::regex reVersionParse("(\\d+)\\.(\\d+)(?:\\.(\\d+))?(?:[\\.\\-\\w](\\d+))?(?:[\\.\\-\\w]([\\w\\.\\-]+))?"); 

/**
 * Constructor of version class
 */
HypervisorVersion::HypervisorVersion( const std::string& verString ) {
    CRASH_REPORT_BEGIN;

    // Set value
    set( verString );

    CRASH_REPORT_END;
}


/**
 * Set a value to the specified version construct
 */
void HypervisorVersion::set( const std::string & version ) {
    CRASH_REPORT_BEGIN;

    // Reset values
    this->major = 0;
    this->minor = 0;
    this->build = 0;
    this->revision = 0;
    this->misc = "";
    this->verString = "";
    this->isDefined = false;

    // Try to match the expression
    boost::smatch matches;
    if (boost::regex_match(version, matches, reVersionParse, boost::match_extra)) {

        // Get the entire matched string
        string stringMatch(matches[0].first, matches[0].second);
        this->verString = stringMatch;

        // Get major/minor
        string strMajor(matches[1].first, matches[1].second);
        this->major = ston<int>( strMajor );
        string strMinor(matches[2].first, matches[2].second);
        this->minor = ston<int>( strMinor );

        // Get build
        string strBuild(matches[3].first, matches[3].second);
        this->build = ston<int>( strBuild );

        // Get revision
        string strRevision(matches[4].first, matches[4].second);
        this->revision = ston<int>( strRevision );

        // Get misc
        string strMisc(matches[5].first, matches[5].second);
        this->misc = strMisc;

        // Mark as defined
        this->isDefined = true;

    }

    CRASH_REPORT_END;
}

/**
 * Return if a version is defined
 */
bool HypervisorVersion::defined() {
    return isDefined;
}

/**
 * Compare to the given revision
 */
int HypervisorVersion::compare( const HypervisorVersion& version ) {
    CRASH_REPORT_BEGIN;

    if ( (version.minor == minor) && (version.major == major)) {
        
        // The rest the same?
        if ((version.revision == revision) && (version.build == build)) {
            return 0;
        }

        // Do first-level comparison on build numbers only if we have them both
        // (Build wins over revision because 'build' refers to a built sofware, while 'revision' to
        //  the repository version)
        if ((version.build != 0) && (build != 0)) {
            if (version.build > build) {
                return 1;
            } else if (version.build < build) {
                return -1;
            }
        }

        // Do second-level comparison on revision numbers only if we have them both
        if ((version.revision != 0) && (revision != 0)) {
            if (version.revision > revision) {
                return 1;
            } else if (version.revision < revision) {
                return -1;
            }
        }

        // Otherwise they are the same
        return 0;

    } else {

        if (version.major == major) {
            // Major same -> Bigger minor always wins
            if (version.minor > minor) {
                return 1;
            } else {
                return -1;
            }
        } else {
            // Bigger major always wins
            if (version.major > major) {
                return 1;
            } else {
                return -1;
            }
        }
    }

    CRASH_REPORT_END;
}

/**
 * Compare to the given string
 */
int HypervisorVersion::compareStr( const std::string& version ) {
    CRASH_REPORT_BEGIN;

    // Create a version object and compare
    HypervisorVersion compareVer(version);
    return compare( compareVer );

    CRASH_REPORT_END;
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
    CRASH_REPORT_BEGIN;
    pf->doing("Extracting compressed disk");
    CRASH_REPORT_END;
}

/**
 * Decompress phase
 */
int __diskExtract( const std::string& sGZOutput, const std::string& checksum, const std::string& sOutput, const VariableTaskPtr & pf ) {
    CRASH_REPORT_BEGIN;
    std::string sChecksum;
    int res;
    
    // Start pf
    if (pf) pf->doing("Extracting disk");

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

/**
 * Measure the resources from the sessions
 */
int HVInstance::getUsage( HVINFO_RES * resCount ) { 
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
int HVInstance::buildContextISO ( std::string userData, std::string * filename ) {
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
int HVInstance::buildFloppyIO ( std::string userData, std::string * filename ) {
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
std::string HVInstance::cernVMVersion( std::string filename ) {
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
int HVInstance::cernVMCached( std::string version, std::string * filename ) {
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
 * Reusable chunk of code to download a SHA256 checksum file
 */
int __downloadChecksum( const std::string & checksumURL, const std::string & sOutChecksum, 
                        const VariableTaskPtr& pfDownload, const FiniteTaskPtr & pf,
                        const DownloadProviderPtr& downloadProvider,
                        const int retries, std::string * sChecksumString ) {

    char linebuf[CHECKSUM_LENGTH+1];
    bool bChecksumOK = false;
    int ans;

    // Start checksum file download and validation
    if (pf) pf->doing("Preparing checksum information");
    for (int i=0; i<retries; i++) {
externalContinue:

        // (1) If file does not exist, download it
        if (!file_exists(sOutChecksum)) {

            // Restart VariableTaskPtr
            if (pfDownload) pfDownload->restart("Downloading checksum file");

            // Download file
            ans = downloadProvider->downloadFile( checksumURL, sOutChecksum, pfDownload );
            if (ans != HVE_OK) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Error while downloading. Will retry.");
                ::remove( sOutChecksum.c_str());
                continue;
            }

        }

        // (2) File exists, validate contents
        if (file_exists(sOutChecksum)) {

            // Try to open file
            std::ifstream ifs( sOutChecksum.c_str(), std::ifstream::in );
            if (!ifs.good()) {
                // Could not open file. That's a fatal error
                if (pf) pf->fail("Unable to read local checksum file", HVE_IO_ERROR);
                return HVE_IO_ERROR;
            }

            // Read 64 characters 
            ifs.getline(linebuf, CHECKSUM_LENGTH+1);
            linebuf[CHECKSUM_LENGTH] = 0;
            *sChecksumString = linebuf;
            ifs.close();

            // Validate the length of the checksum
            if (sChecksumString->length() < CHECKSUM_LENGTH) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Checksum length not validated. Re-downloading");
                ::remove( sOutChecksum.c_str());
                continue;
            }

            // Validate the integrity of the checksum buffer
            const std::string allowedChars = "0123456789abcdef";
            for (int j=0; j<CHECKSUM_LENGTH; j++) {
                if (allowedChars.find( (*sChecksumString)[j] ) == std::string::npos) {
                    // Invalid contents. Erase and re-download
                    if (pf) pf->doing("Checksum contents not validated. Re-downloading");
                    ::remove( sOutChecksum.c_str());
                    goto externalContinue;
                }
            }

            // Looks good
            if (pf) pf->done("Checksum file in place");
            bChecksumOK = true;
            break;
        }

    }

    // Check if we ran out of retries while trying 
    if (!bChecksumOK) {
        if (pf) pf->fail("Unable to download checksum file", HVE_IO_ERROR);
        ::remove( sOutChecksum.c_str());
        return HVE_IO_ERROR;
    } else {
        if (pf) pf->done("Checksum data obtained");
    }

    // Return OK
    return HVE_OK;

}

/**
 * Reusable chunk of code to download a SHA256 checksum file
 */
int __downloadFile( const std::string & fileURL, const std::string & sOutFilename, 
                    const VariableTaskPtr& pfDownload, const FiniteTaskPtr & pf,
                    const DownloadProviderPtr& downloadProvider, const std::string& sChecksumString,
                    const int retries ) {

    bool bFileOK = false;
    int ans;

    // Start actual file download and validation
    if (pf) pf->doing("Preparing file download");
    for (int i=0; i<retries; i++) {

        // (3) If file does not exist, download it
        if (!file_exists(sOutFilename)) {

            // Restart VariableTaskPtr
            if (pfDownload) pfDownload->restart("Downloading file");

            // Download file
            ans = downloadProvider->downloadFile( fileURL, sOutFilename, pfDownload );
            if (ans != HVE_OK) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Error while downloading. Will retry.");
                ::remove( sOutFilename.c_str());
                continue;
            }

        }

        // (4) File exists, validate contents
        if (file_exists(sOutFilename)) {

            // Calculate checksum
            std::string     sChecksumFile = "";
            sha256_file( sOutFilename, &sChecksumFile );

            // Compare checksums
            if (sChecksumFile.compare( sChecksumString ) != 0) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Downloaded file checksum invalid. Re-downloading.");
                ::remove( sOutFilename.c_str());
                continue;
            }

            // Looks good
            if (pf) pf->done("Downloaded file in place");
            bFileOK = true;
            break;
        }

    }

    // Check if we ran out of retries while trying 
    if (!bFileOK) {
        if (pf) pf->fail("Unable to download file", HVE_IO_ERROR);
        ::remove( sOutFilename.c_str());
        return HVE_IO_ERROR;
    } else {
        if (pf) pf->done("File downloaded");
    }

    // Return OK
    return HVE_OK;

}

/**
 * Download an arbitrary file and validate it against a checksum
 * file, both provided as URLs
 */
int HVInstance::downloadFileURL ( const std::string & fileURL, const std::string & checksumURL, std::string * filename, const FiniteTaskPtr & pf, const int retries ) {
    CRASH_REPORT_BEGIN;
    int ans;

    // Calculate filename and full URL hash
    std::string     sOutFilenameHash;
    std::string     sOutFilename = getURLFilename(fileURL);
    sha256_buffer( fileURL, &sOutFilenameHash );

    // Calculate full path for the output file
    sOutFilename = dirData + "/cache/" + sOutFilenameHash + "-" + sOutFilename;

    // Calculate full path for the checksum file
    std::string     sOutChecksum = sOutFilename + ".sha256";
    std::string     sChecksumString = "";

    // Prepare progress objects
    VariableTaskPtr   pfDownload;
    if (pf) pf->setMax(5);

    // Download checksum
    pfDownload = pf->begin<VariableTask>("Downloading Checksum");    
    ans = __downloadChecksum(
            checksumURL, sOutChecksum, pfDownload, pf, this->downloadProvider,
            retries, &sChecksumString
        );
    if (ans != HVE_OK) return ans;

    // Download file
    pfDownload = pf->begin<VariableTask>("Downloading file");    
    ans = __downloadFile(
            fileURL, sOutFilename, pfDownload, pf, this->downloadProvider,
            sChecksumString, retries
        );
    if (ans != HVE_OK) return ans;

    // Update the string
    if (pf) pf->complete("File download completed");
    *filename = sOutFilename;
    
    // Return OK
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Download an arbitrary file and validate it against a checksum
 * string specified in parameter
 */
int HVInstance::downloadFile ( const std::string & fileURL, const std::string & checksumString, std::string * filename, const FiniteTaskPtr & pf, const int retries ) {
    CRASH_REPORT_BEGIN;
    int ans;

    // Calculate filename and full URL hash
    std::string     sOutFilenameHash;
    std::string     sOutFilename = getURLFilename(fileURL);
    sha256_buffer( fileURL, &sOutFilenameHash );

    // Calculate full path for the output file
    sOutFilename = dirData + "/cache/" + sOutFilenameHash + "-" + sOutFilename;

    // Prepare progress objects
    VariableTaskPtr   pfDownload;
    if (pf) pf->setMax(3);

    // Download file
    pfDownload = pf->begin<VariableTask>("Downloading file");    
    ans = __downloadFile(
            fileURL, sOutFilename, pfDownload, pf, this->downloadProvider,
            checksumString, retries
        );
    if (ans != HVE_OK) return ans;

    // Update the string
    if (pf) pf->complete("File download completed");
    *filename = sOutFilename;
    
    // Return OK
    return HVE_OK;
    CRASH_REPORT_END;
}

/**
 * Download a gzip-compressed arbitrary file and validate it's extracted
 * contents against a checksum string specified in parameter
 */
int HVInstance::downloadFileGZ ( const std::string & fileURL, const std::string & checksumString, std::string * filename, const FiniteTaskPtr & pf, const int retries ) {
    CRASH_REPORT_BEGIN;
    int ans;

    // Calculate filename and full URL hash
    std::string     sOutFilenameHash;
    std::string     sOutFilename = getURLFilename(fileURL);
    sha256_buffer( fileURL, &sOutFilenameHash );

    // Strip-out .gz from the extension and store it to a different file
    std::string     sExtractedFilename = sOutFilename;
    size_t gzPos;
    if ( (gzPos = sExtractedFilename.find(".gz")) != std::string::npos )
        sExtractedFilename = sExtractedFilename.substr(0, gzPos);

    // Calculate full path for the output and extract file
    sOutFilename = dirData + "/cache/" + sOutFilenameHash + "-" + sOutFilename;
    sExtractedFilename = dirData + "/cache/" + sOutFilenameHash + "-" + sExtractedFilename;

    // Prepare progress objects
    VariableTaskPtr pfDownload;
    if (pf) pf->setMax(3);

    // File OK flag
    bool            bFileOK = false;

    // Start actual file download and validation
    pfDownload = pf->begin<VariableTask>("Downloading file");
    for (int i=0; i<retries; i++) {

        // (1) If no file exists, download compressed file
        if ( !file_exists(sExtractedFilename) && !file_exists(sOutFilename) ) {

            // Restart VariableTaskPtr
            if (pfDownload) pfDownload->restart("Downloading compressed file");

            // Download file
            ans = downloadProvider->downloadFile( fileURL, sOutFilename, pfDownload );
            if (ans != HVE_OK) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Error while downloading. Will retry.");
                ::remove( sOutFilename.c_str());
                continue;
            }

        }

        // (2) If input file exists, but no extracted file exists, decompress
        if ( !file_exists(sExtractedFilename) && file_exists(sOutFilename) ) {

            // Decompress GZip file
            if (pf) pf->doing("Extracting file");
            decompressFile( sOutFilename, sExtractedFilename );

        }

        // (3) If extracted AND source file exists, validate checksum and delete source file
        if ( file_exists(sExtractedFilename) && file_exists(sOutFilename) ) {

            // Calculate checksum
            std::string     sChecksumFile = "";
            sha256_file( sExtractedFilename, &sChecksumFile );

            // Compare checksums
            if (sChecksumFile.compare( checksumString ) != 0) {
                // Invalid contents. Erase and re-download
                if (pf) pf->doing("Extracted file checksum invalid. Re-downloading.");
                ::remove( sExtractedFilename.c_str());
                continue;
            }

            // It was extracted. Remove compressed, downloaded file
            ::remove( sOutFilename.c_str());

        }

        // (4) If only extracted file exists, it means that validation was successful
        if ( file_exists(sExtractedFilename) && !file_exists(sOutFilename) ) {

            // We are good
            if (pf) pf->done("Downloaded file in place");
            bFileOK = true;
            break;

        }

    }

    // Check if we ran out of retries while trying 
    if (!bFileOK) {
        if (pf) pf->fail("Unable to download file", HVE_IO_ERROR);
        ::remove( sOutFilename.c_str());
        ::remove( sExtractedFilename.c_str());
        return HVE_IO_ERROR;
    } else {
        if (pf) pf->done("File downloaded");
    }

    // Update the string
    if (pf) pf->complete("File downloaded");
    *filename = sOutFilename;
    
    // Return OK
    return HVE_OK;
    CRASH_REPORT_END;
}


/**
 * Download the specified CernVM version
 */
/*
int HVInstance::cernVMDownload( std::string version, std::string * filename, const FiniteTaskPtr & pf, std::string flavor, std::string arch ) {
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
*/

/**
 * Download the specified generic, compressed disk image
 */
/*
int HVInstance::diskImageDownload( std::string url, std::string checksum, std::string * filename, const FiniteTaskPtr & pf ) {
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
*/

/**
 * Cross-platform exec and return for the hypervisor control binary
 */
int HVInstance::exec( string args, vector<string> * stdoutList, string * stderrMsg, const SysExecConfig& config ) {
    CRASH_REPORT_BEGIN;
    int execRes = 0;

    /* If retries is negative, do not monitor the execution */
    if (config.retries < 0) {

        /* Execute asynchronously */
        execRes = sysExecAsync( this->hvBinary, args );

    } else {
    
        /* Execute */
        string execError;
        execRes = sysExec( this->hvBinary, args, stdoutList, &execError, config );
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
HVInstance::HVInstance() : version(""), sessions(), openSessions(), userInteraction(), downloadProvider() {
    CRASH_REPORT_BEGIN;
    this->sessionID = 1;
    
    // Pick a system folder to store persistent information
    this->dirData = getAppDataPath();
    this->dirDataCache = this->dirData + "/cache";
    
    // Unless overriden use the default downloadProvider and 
    // userInteraction pointers
    downloadProvider = DownloadProvider::Default();
    userInteraction = UserInteraction::Default();
    
    CRASH_REPORT_END;
};

/**
 * Check the status of the session. It returns the following values:
 *  0  - Does not exist
 *  1  - Exist and has a valid key
 *  2  - Exists and has an invalid key
 *  <0 - An error occured
 */
int HVInstance::sessionValidate ( const ParameterMapPtr& parameters ) {
    CRASH_REPORT_BEGIN;

    // Extract name and key
    std::string name = parameters->get("name");
    if (name.empty()) {
        CVMWA_LOG("Error", "Missing 'name' parameter on sessionValidate" );
        return HVE_NOT_VALIDATED;
    }
    std::string key = parameters->get("key");
    if (key.empty()) {
        CVMWA_LOG("Error", "Missing 'key' parameter on sessionValidate" );
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
 * Open or reuse a hypervisor session
 */
HVSessionPtr HVInstance::sessionOpen( const ParameterMapPtr& parameters, const FiniteTaskPtr & pf ) { 
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
        if (sess->parameters->get("key","").compare(keyHash) != 0) {
            // Exists but the password is invalid
            return voidPtr;
        }
    } else {

        // Otherwise, allocate one
        sess = this->allocateSession();
        if (!sess) return voidPtr;

    }

    // Populate parameters
    sess->parameters->fromParameters( parameters );
    sess->parameters->set("key", keyHash); // (Replace with it's crypto-hash version)

    // Store it on open sessions
    openSessions.push_back( sess );
    
    // Return the handler
    return sess;
    CRASH_REPORT_END;
}

/**
 * Return a session object by locating it by name
 */
HVSessionPtr HVInstance::sessionByName ( const std::string& name ) {
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
 * Check if we need to start or stop the daemon 
 */
int HVInstance::checkDaemonNeed() {
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
void HVInstance::setDownloadProvider( DownloadProviderPtr p ) { 
    CRASH_REPORT_BEGIN;
    this->downloadProvider = p;
    CRASH_REPORT_END;
};

/**
 * Search the system's folders and try to detect what hypervisor
 * the user has installed and it will then populate the Hypervisor Config
 * structure that was passed to it.
 */
HVInstancePtr detectHypervisor() {
    CRASH_REPORT_BEGIN;
    HVInstancePtr hv;

    /* 1) Look for Virtualbox */
    hv = vboxDetect();
    if (hv) return hv;

    /* 2) Check for other hypervisors */
    // TODO: Implement this
    
    /* No hypervisor found */
    return hv;
    CRASH_REPORT_END;
}

/**
 * Install hypervisor
 */
int installHypervisor( const DownloadProviderPtr& downloadProvider, const UserInteractionPtr & ui, const FiniteTaskPtr & pf, int retries ) {
    
    // The only hypervisor we currently support is VirtualBox
    return vboxInstall( downloadProvider, ui, pf, retries );

}
