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

#include <openssl/sha.h>

#include "Utilities.h"
#include "Hypervisor.h"

using namespace std;

/* Base64 Helper */
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

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
        
        /* Delete the file that windows create automatically */
        remove( tmpName );
        
        /* Create string with the appropriate suffix */
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
    if(!buffer) {
        fclose(file);
        return -1;
    }
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
    
    // Throttle events on 2 per second
    if ((dlnow != dltotal) && ((getMillis() - fb->lastEventTime) < 500)) return 0;
    fb->lastEventTime = getMillis();

    // Calculate percentage
    int ipos = fb->min;
    if (dltotal != 0) {
        double pos = (fb->max - fb->min) * dlnow / dltotal;
        ipos += (int)pos;
    }
    
    // Callback
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
            fb->lastEventTime = getMillis();
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
 * Check if the given port is open
 */
bool isPortOpen( const char * host, int port ) {
    SOCKET sock;

    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_family = AF_INET;
    client.sin_port = htons( port );
    client.sin_addr.s_addr = inet_addr( host );
    
    sock = (int) socket(AF_INET, SOCK_STREAM, 0);
    int result = connect(sock, (struct sockaddr *) &client,sizeof(client));

    if (result < 0) {
        return false;
    } else {
        #ifdef _WIN32
            closesocket(sock);
        #else
            ::close(sock);
        #endif
        return true;
    }
}

/**
 * Dump a map structure
 */
void mapDump(map<string, string> m) {
    for (std::map<string, string>::iterator it=m.begin(); it!=m.end(); ++it) {
        string k = (*it).first;
        string v = (*it).second;
        cout << k << " => " << v << "\n";
    }
};

/**
 * Hex dump of the given buffer
 */
void hexDump (const char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char *) addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

/**
 * Check if the given string contains only the specified chars
 */
bool isSanitized( std::string * check, std::string chars ) {
    for (int i=0; i<check->length(); i++) {
        if (chars.find( check->at(i) ) == string::npos) {
            return false;
        }
    }
    return true;
}

#ifdef __linux__

/**
 * Identify the linux environment and return it's id
 * (Used by the hypervisor installation system in order to pick the appropriate binary)
 */
void getLinuxInfo ( LINUX_INFO * info ) {
    std::vector< std::string > vLines;
    
    // Check if we have gksudo
    info->hasGKSudo = file_exists("/usr/bin/gksudo");
    
    // Identify pakage manager
    info->osPackageManager = ARCHIVE_NONE;
    if (file_exists("/usr/bin/dpkg")) {
        info->osPackageManager = ARCHIVE_APT;
    } else if (file_exists("/usr/bin/yum")) {
        info->osPackageManager = ARCHIVE_YUM;
    }
    
    // Use lsb_release to identify the linux version
    info->osDistID = "generic";
    if (file_exists("/usr/bin/lsb_release")) {
        
        // First, get release
        std::string cmdline = "/usr/bin/lsb_release -i -s";
        if (sysExec( cmdline, &vLines ) == 0) {
            if (vLines[0].compare("n/a") != 0) {
                info->osDistID = vLines[0];
            }
        }
        
        // Then get codename
        cmdline = "/usr/sbin/lsb_release -c -s";
        if (sysExec( cmdline, &vLines ) == 0) {
            if (vLines[0].compare("n/a") != 0) {
                // Put separator and get version
                info->osDistID += "-";
                info->osDistID += vLines[0];
            }
        }
        
        // To lower case
        std::transform(
            info->osDistID.begin(), 
            info->osDistID.end(), 
            info->osDistID.begin(), 
            ::tolower);
        
    }
}

#endif
