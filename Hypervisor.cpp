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
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <vector>

#include "Hypervisor.h"
#include "Virtualbox.h"
#include "contextiso.h"

using namespace std;

/* Incomplete type placeholders */
int Hypervisor::loadSessions()                                      { return HVE_NOT_IMPLEMENTED; }
int HVSession::pause()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::close()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::stop()                                               { return HVE_NOT_IMPLEMENTED; }
int HVSession::resume()                                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::reset()                                              { return HVE_NOT_IMPLEMENTED; }
int HVSession::open( int cpus, int memory, int disk, std::string cvmVersion ) 
                                                                    { return HVE_NOT_IMPLEMENTED; }
int HVSession::start( std::string userData )                        { return HVE_NOT_IMPLEMENTED; }
int HVSession::setExecutionCap(int cap)                             { return HVE_NOT_IMPLEMENTED; }
int HVSession::setProperty( std::string name, std::string key )     { return HVE_NOT_IMPLEMENTED; }
std::string HVSession::getProperty( std::string name )              { return ""; }

/* Base64 Helper */
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

/**
 * Check if the given file exists and is readible
 */
inline bool file_exists( string path_to_file )
    { return ifstream( (const char*)path_to_file.c_str()) ; }

/**
 * Convert an std::string to a number
 */
template <typename T> T ston( const string &Text ) {
	stringstream ss(Text); T result;
	return ss >> result ? result : 0;
}

/**
 * Helper function for writing data
 */
size_t __curl_write_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
size_t __curl_write_string(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
 * Small function to encode data to base64
 */
std::string base64_encode(char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];
    while((i++ < 3))
      ret += '=';
  }
  
  return ret;
}


/**
 * Generic purpose download to file function
 */
int downloadFile( std::string url, std::string target ) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(target.c_str(),"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __curl_write_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
        if (res != 0) {
            remove( target.c_str() );
            return HVE_IO_ERROR;
        } else {
            return HVE_OK;
        }
    } else {
        return HVE_QUERY_ERROR;
    }
};

/**
 * General purpose download to buffer
 */
int downloadText( std::string url, std::string * buffer ) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __curl_write_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        if (res != 0) {
            return HVE_IO_ERROR;
        } else {
            return HVE_OK;
        }
    } else {
        return HVE_QUERY_ERROR;
    }
};

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
    if (error == -100) return "Not implemented";
    return "Unknown error";
};


/**
 * Return a temporary filename
 */
std::string Hypervisor::getTmpFile( std::string suffix ) {
    char * tmp = tmpnam(NULL);
    string tmpFile = tmp;
    tmpFile += suffix;
    return tmpFile;
}

/**
 * Use LibcontextISO to create a cd-rom for this VM
 */
int Hypervisor::buildContextISO ( std::string userData, std::string * filename ) {
    ofstream isoFile;
    string iso = this->getTmpFile(".iso");
    
    const char * cData = userData.c_str();
    string ctxFileContents = base64_encode( cData, userData.length() );
    ctxFileContents = "EC2_USER_DATA=\"" +ctxFileContents + "\"\nONE_CONTEXT_PATH=\"/var/lib/amiconfig\"\n";
    const char * fData = ctxFileContents.c_str();
    
    char * data = build_simple_cdrom( "CONTEXT_INFO", "CONTEXT.SH", fData, ctxFileContents.length() );
    isoFile.open( iso.c_str() );
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
int Hypervisor::cernVMDownload( std::string version, std::string * filename ) {
    string sURL = "http://cernvm.cern.ch/portal/sites/cernvm.cern.ch/files/ucernvm-" + version + ".iso";
    string sOutput = this->dirDataCache + "/ucernvm-" + version + ".iso";
    *filename = sOutput;
    if (file_exists(sOutput)) {
        return 0;
    } else {
        return downloadFile(sURL, sOutput);
    }
};

/**
 * Cross-platform exec and return for the hypervisor control binary
 */
int Hypervisor::exec( string args, vector<string> * stdout ) {

    int ret;
    char data[1035];
    string cmdline( this->hvBinary );
    string item;
    string rawStdout = "";
    FILE *fp;

    /* Build cmdline */
    cmdline += " " + args;

    /* Open process and red contents using the POSIX pipe interface */
    #ifdef _WIN32
        fp = _popen(cmdline.c_str(), "r");
    #else
        fp = popen(cmdline.c_str(), "r");
    #endif

    /* Check for error */
    if (fp == NULL) return HVE_IO_ERROR;

    /* Read the output a line at a time - output it. */
    if (stdout != NULL) {
        
        /* Read to buffer */
        while (fgets(data, sizeof(data)-1, fp) != NULL) {
            rawStdout += data;
        }
        
        /* Pass output into an input stream */
        istringstream ss(rawStdout);

        /* Split new lines and store them in the vector */
        stdout->clear();
        while (getline(ss, item)) {
            stdout->push_back(item);
        }
        
    }

    /* close */
    #ifdef _WIN32
        ret = _pclose(fp);
    #else
        ret = pclose(fp);
    #endif

    /* Return exit code */
    return ret;
}

/**
 * Initialize hypervisor 
 */
Hypervisor::Hypervisor() {
    this->sessionID = 1;
    std::string homeDir;
    
    /* Pick a system folder to store information  */
    #ifdef _WIN32
    homeDir = getenv("APPDATA");
    homeDir += "/CernVM";
    _mkdir(homeDir.c_str());
    homeDir += "/WebAPI";
    _mkdir(homeDir.c_str());
    this->dirDataCache = homeDir + "/cache";
    _mkdir(this->dirDataCache.c_str());
    this->dirData = homeDir;
    #endif
    
    #if defined(__APPLE__) && defined(__MACH__)
    struct passwd *p = getpwuid(getuid());
    char *home = p->pw_dir;
    homeDir = home;
    homeDir += "/Library/Application Support/CernVM";
    mkdir(homeDir.c_str(), 0777);
    homeDir += "/WebAPI";
    mkdir(homeDir.c_str(), 0777);
    this->dirDataCache = homeDir + "/cache";
    mkdir(this->dirDataCache.c_str(), 0777);
    this->dirData = homeDir;
    #endif
    
    #ifdef __linux__
    struct passwd *p = getpwuid(getuid());
    char *home = p->pw_dir;
    homeDir = home;
    homeDir += "/.cernvm";
    mkdir(homeDir.c_str(), 0777);
    homeDir += "/WebAPI";
    mkdir(homeDir.c_str(), 0777);
    this->dirDataCache = homeDir + "/cache";
    mkdir(this->dirDataCache.c_str(), 0777);
    this->dirData = homeDir;
    #endif
        
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
 * Open or reuse a hypervisor session
 */
HVSession * Hypervisor::sessionOpen( std::string name, std::string key ) { 
    
    /* Check for running sessions with the given credentials */
    std::cout << "Checking sessions (" << this->sessions.size() << ")\n";
    for (vector<HVSession*>::iterator i = this->sessions.begin(); i != this->sessions.end(); i++) {
        HVSession* sess = *i;
        std::cout << "Checking session name=" << sess->name << ", key=" << sess->key << ", uuid=" << ((VBoxSession*)sess)->uuid << ", state=" << sess->state << "\n";
        
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
    paths.push_bach( "/opt/VirtualBox" );
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
            return hv;
        }
        #endif
        #ifdef __linux__
        bin = p + "/bin/VBoxManage";
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
            return hv;
        }
        #endif
        
    }
    
    /* No hypervisor found */
    return NULL;
}
