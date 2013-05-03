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
#include <cmath>

#include "openssl/sha.h"
#include "Hypervisor.h"
#include "Virtualbox.h"
#include "contextiso.h"

using namespace std;

/* Incomplete type placeholders */
int Hypervisor::loadSessions()                                      { return HVE_NOT_IMPLEMENTED; }
int Hypervisor::getCapabilities ( HVINFO_CAPS * )                   { return HVE_NOT_IMPLEMENTED; };
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

template <typename T> T hex_ston( const std::string &Text ) {
    stringstream ss; T result; ss << std::hex << Text;
    return ss >> result ? result : 0;
}

/**
 * LINK: Expose <int> implementations for the above templates
 */
template int hex_ston<int>( const std::string &Text );
template long hex_ston<long>( const std::string &Text );
template int ston<int>( const std::string &Text );
template long ston<long>( const std::string &Text );

/**
 * Split the given line into a key and value using the delimited provided
 */
int getKV( string line, string * key, string * value, unsigned char delim, int offset ) {
    size_t a = line.find( delim, offset );
    if (a == string::npos) return 0;
    *key = line.substr(offset, a-offset);
    size_t b = a+1;
    while ( ((line[b] == ' ') || (line[b] == '\t')) && (b<line.length())) b++;
    *value = line.substr(b, string::npos);
    return a;
}

/**
 * Split the given string into 
 */
int trimSplit( std::string * line, std::vector< std::string > * parts, std::string split, std::string trim ) {
    size_t i, pos, nextPos, ofs = 0;
    parts->clear();
    while (ofs < line->length()) {
        
        // Find the closest occurance of 'split' delimiters
        nextPos = line->length();
        for (i=0; i<split.length(); i++) {
            pos = line->find( split[i], ofs+1 );
            if ((pos != string::npos) && (pos < nextPos)) nextPos = pos;
        }
        
        // Get part
        parts->push_back( line->substr(ofs, nextPos-ofs) );
        
        // Cleanup and move forward
        if (nextPos == line->length()) break;
        bool ws = true; nextPos++; // Skip delimiter
        while (ws && (nextPos < line->length())) {
            ws = false;
            if (trim.find(line->at(nextPos)) != string::npos) {
                ws = true;
                nextPos++;
            }
        }
        ofs = nextPos;
        
    }
    
    return parts->size();
}

/**
 * Read the specified list of lines and create a map
 */
int parseLine( std::vector< std::string > * lines, std::map< std::string, std::string > * map, std::string csplit, std::string ctrim, size_t key, size_t value ) {
    string line;
    vector<string> parts;
    map->clear();
    for (vector<string>::iterator i = lines->begin(); i != lines->end(); i++) {
        line = *i;
        trimSplit( &line, &parts, csplit, ctrim );
        if ((key < parts.size()) && (value < parts.size())) {
            map->insert(std::pair<string,string>(parts[key], parts[value]));
        }
    }
    return HVE_OK;
}

/**
 * Split lines from the given raw buffer
 */
void splitLines( string rawString, vector<string> * out ) {
    
    /* Pass output into an input stream */
    istringstream ss( rawString );

    /* Split new lines and store them in the vector */
    int iTrim;
    string item;
    out->clear();
    while (getline(ss, item)) {
        
        /* Trim junk */
        iTrim = item.find('\r');
        if (iTrim != string::npos)
            item = item.substr(0, iTrim);
        iTrim = item.find('\n');
        if (iTrim != string::npos)
            item = item.substr(0, iTrim);
        
        /* Push back */
        out->push_back(item);
    }
    
}

/**
 * Tokenize a key-value like output from VBoxManage into an easy-to-use hashmap
 */
map<string, string> tokenize( vector<string> * lines, char delim ) {
    map<string, string> ans;
    string line, key, value;
    for (vector<string>::iterator i = lines->begin(); i != lines->end(); i++) {
        line = *i;
        if (line.find(delim) != string::npos) {
            getKV(line, &key, &value, delim, 0);
            if (ans.find(key) == ans.end()) {
                ans[key] = value;
            }
        }
    }
    return ans;
};

/**
 * Tokenize a list of repeating key-value groups
 */
vector< map<string, string> > tokenizeList( vector<string> * lines, char delim ) {
    vector< map<string, string> > ans;
    map<string, string> row;
    string line, key, value;
    for (vector<string>::iterator i = lines->begin(); i != lines->end(); i++) {
        line = *i;
        if (line.find(delim) != string::npos) {
            getKV(line, &key, &value, delim, 0);
            row[key] = value;
        } else if (line.length() == 0) { // Empty line -> List delimiter
            ans.push_back(row);
            row.clear();
        }
    }
    if (!row.empty()) ans.push_back(row);
    return ans;
};

/**
 * Return a temporary filename
 */
std::string getTmpFile( std::string suffix ) {
    #ifdef _WIN32
        DWORD wRet;
        int lRet;
        char tmpPath[MAX_PATH];
        char tmpName[MAX_PATH];
        
        /* Get temporary folder */
        wRet = GetTempPathA( MAX_PATH, tmpPath );
        if (wRet == 0) return "";
        
        /* Get temporary file name */
        lRet = GetTempFileNameA( tmpPath, "cvm", 0, tmpName );
        if (lRet == 0) return "";
        
        /* Create string */
        string tmpStr( tmpName );
        tmpStr += suffix;
        return tmpStr;
    
    #else
        char * tmp = tmpnam(NULL);
        string tmpFile = tmp;
        tmpFile += suffix;
        return tmpFile;
    #endif
}

/**
 * Cross-platform exec and return function
 */
#ifndef _WIN32
int sysExec( string cmdline, vector<string> * stdoutList ) {
    
    int ret;
    char data[1035];
    string item;
    string rawStdout = "";
    FILE *fp;

    /* Open process and red contents using the POSIX pipe interface */
    fp = popen(cmdline.c_str(), "r");

    /* Check for error */
    if (fp == NULL) return HVE_IO_ERROR;

    /* Read the output a line at a time - output it. */
    if (stdoutList != NULL) {
        
        /* Read to buffer */
        while (fgets(data, sizeof(data)-1, fp) != NULL) {
            rawStdout += data;
        }
        
        /* Split lines into stdout */
        splitLines( rawStdout, stdoutList );
        
    }

    /* close */
    ret = pclose(fp);

    /* Return exit code */
    return ret;
}
#else
int sysExec( string cmdline, vector<string> * stdoutList ) {

	HANDLE g_hChildStdOut_Rd = NULL;
	HANDLE g_hChildStdOut_Wr = NULL;
	DWORD ret, dwRead;
	CHAR chBuf[4096];
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	BOOL bSuccess = FALSE;
	string rawStdout;

	SECURITY_ATTRIBUTES sAttr;
	sAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
	sAttr.bInheritHandle = TRUE;
	sAttr.lpSecurityDescriptor = NULL;

    /* Create STDOUT pipe */
	if (!CreatePipe(&g_hChildStdOut_Rd, &g_hChildStdOut_Wr, &sAttr, 0)) 
		return HVE_IO_ERROR;
	if (!SetHandleInformation(g_hChildStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
		return HVE_IO_ERROR;

    /* Clean structures */
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory( &siStartInfo, sizeof( STARTUPINFOA ));

    /* Prepare startup information */
	siStartInfo.cb = sizeof(STARTUPINFOA);
	siStartInfo.hStdOutput = g_hChildStdOut_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    /* Create process */
	if (!CreateProcessA(
		NULL,
		(LPSTR)cmdline.c_str(),
		NULL,
		NULL,
		TRUE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo)) return HVE_IO_ERROR;

    /* Process STDOUT */
	CloseHandle( g_hChildStdOut_Wr );
	if ( stdoutList != NULL ) {
	    
	    /* Read to buffer */
    	for (;;) {
    		bSuccess = ReadFile( g_hChildStdOut_Rd, chBuf, 4096, &dwRead, NULL);
    		if ( !bSuccess || dwRead == 0 ) break;
    		rawStdout.append( chBuf, dwRead );
    	}
    	
	    /* Split lines into stdout */
        splitLines( rawStdout, stdoutList );
        
	}
	CloseHandle( g_hChildStdOut_Rd );

    /* Wait for completion */
	WaitForSingleObject( piProcInfo.hProcess, INFINITE );
	GetExitCodeProcess( piProcInfo.hProcess, &ret );

    /* Close hanles */
	CloseHandle( piProcInfo.hProcess );
	CloseHandle( piProcInfo.hThread );

    /* Return exit code */
	return ret;
}
#endif

/**
 * Sha256 from binary to hex
 */
void __sha256_hash_string( unsigned char * hash, char * outputBuffer) {
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

/**
 * OpenSSL 
 */
int sha256_file( string path, string * checksum ) {
    
    FILE *file = fopen(path.c_str(), "rb");
    if(!file) return -534;

    char outputBuffer[65];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = (unsigned char *) malloc(bufSize);
    int bytesRead = 0;
    if(!buffer) return -1;
    while((bytesRead = fread(buffer, 1, bufSize, file))) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    __sha256_hash_string( hash, outputBuffer);
    fclose(file);
    free(buffer);
    *checksum = outputBuffer;
    
    return 0;

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
 * Helper function for delegating progress
 */
int __curl_progress_proxy(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    HVPROGRESS_FEEDBACK * fb = (HVPROGRESS_FEEDBACK *) clientp;
    int ipos = fb->min;
    if (dltotal != 0) {
        double pos = (fb->max - fb->min) * dlnow / dltotal;
        ipos += (int)pos;
    }
    cout << "INFO: dltotal=" << dltotal << ", dlnow=" << dlnow << ", ultotal=" << ultotal << ", ulnow=" << ulnow << ", ipos=" << ipos << "\n";
    fb->callback( ipos, fb->total, fb->message, fb->data );
    return 0;
};

/**
 * Generic purpose download to file function
 */
int downloadFile( std::string url, std::string target, HVPROGRESS_FEEDBACK * fb ) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(target.c_str(),"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __curl_write_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        if ((fb != NULL) && (fb->callback != NULL)) {
            cout << "INFO: Using feedbac callback\n";
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, __curl_progress_proxy );
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, fb);
        }
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
        curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
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
    if (error == -10) return "Not allowed";
    if (error == -11) return "Not supported";
    if (error == -12) return "Not validated";
    if (error == -20) return "Password denied";
    if (error == -100) return "Not implemented";
    return "Unknown error";
};

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
    
    const char * cData = userData.c_str();
    string ctxFileContents = base64_encode( cData, userData.length() );
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
            hv->hvRoot = p + "/bin";
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
    const string kDownloadUrl = "linux";
    const string kChecksum = "linux-sha256";
    const string kInstallerName = "linux-installer";
	const string kFileExt = ".run";
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

		/* Install using GKSudo */
		string cmdline = "gksudo sh \"" + tmpHypervisorInstall + "\"";
		res = system( cmdline.c_str() );
		if (res < 0) {

			cout << "ERROR: Could not start. Return code: " << res << endl;
			remove( tmpHypervisorInstall.c_str() );
			return HVE_EXTERNAL_ERROR;
		}

    #endif
    
    /**
     * Check if it was successful
     */
    Hypervisor * hv = detectHypervisor();
    if (hv == NULL) {
        cout << "ERROR: Could not install hypervisor!\n";
        return HVE_NOT_VALIDATED;
    };
    
    /**
     * Completed
     */
    return HVE_OK;
    
}
