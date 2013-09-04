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
#include <iterator>
#include <vector>
#include <limits>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include <openssl/sha.h>
#include "zlib.h"

#include "Utilities.h"
#include "Hypervisor.h"

using namespace std;

/* Base64 Helper */
static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

/* Singleton optimization for AppData folder */
std::string appDataDir = "";

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

template <typename T> std::string ntos( T &value ) {
    std::stringstream out; out << value;
    return out.str();
}

/**
 * LINK: Expose <int> implementations for the above templates
 */
template int hex_ston<int>( const std::string &Text );
template long hex_ston<long>( const std::string &Text );
template int ston<int>( const std::string &Text );
template long ston<long>( const std::string &Text );
template size_t ston<size_t>( const std::string &Text );
template std::string ntos<int>( int &value );
template std::string ntos<long>( long &value );
template std::string ntos<size_t>( size_t &value );

/**
 * Get the location of the platform-dependant application data folder
 * This function also builds the required directory structure:
 *
 * + CernVM/
 * +-- WebAPI/
 *   +-- cache/
 *   +-- config/
 *   +-- run/
 *
 */
std::string prepareAppDataPath() {
    std::string homeDir;
    std::string subDir;
    
    /* On windows it goes on AppData */
    #ifdef _WIN32
    homeDir = getenv("APPDATA");
    homeDir += "/CernVM";
    _mkdir(homeDir.c_str());
    homeDir += "/WebAPI";
    _mkdir(homeDir.c_str());
    subDir = homeDir + "/cache";
    _mkdir(subDir.c_str());
    subDir = homeDir + "/run";
    _mkdir(subDir.c_str());
    subDir = homeDir + "/config";
    _mkdir(subDir.c_str());
    #endif
    
    /* On Apple it goes on user's Application Support */
    #if defined(__APPLE__) && defined(__MACH__)
    struct passwd *p = getpwuid(getuid());
    char *home = p->pw_dir;
    homeDir = home;
    homeDir += "/Library/Application Support/CernVM";
    mkdir(homeDir.c_str(), 0777);
    homeDir += "/WebAPI";
    mkdir(homeDir.c_str(), 0777);
    subDir = homeDir + "/cache";
    mkdir(subDir.c_str(), 0777);
    subDir = homeDir + "/run";
    mkdir(subDir.c_str(), 0777);
    subDir = homeDir + "/config";
    mkdir(subDir.c_str(), 0777);
    #endif
    
    /* On linux it goes on the .cernvm dotfolder in user's home dir */
    #ifdef __linux__
    struct passwd *p = getpwuid(getuid());
    char *home = p->pw_dir;
    homeDir = home;
    homeDir += "/.cernvm";
    mkdir(homeDir.c_str(), 0777);
    homeDir += "/WebAPI";
    mkdir(homeDir.c_str(), 0777);
    subDir = homeDir + "/cache";
    mkdir(subDir.c_str(), 0777);
    subDir = homeDir + "/run";
    mkdir(subDir.c_str(), 0777);
    subDir = homeDir + "/config";
    mkdir(subDir.c_str(), 0777);
    #endif
    
    /* Return the home directory */
    return homeDir;
    
}

/**
 * Remove a trailing folder from the given path
 */
std::string stripComponent( std::string path ) {
    /* Find the last trailing slash */
    size_t iPos = path.find_last_of('/');
    if (iPos == std::string::npos) { // Check for windows-like trailing path
        iPos = path.find_last_of('\\');
    }
    
    /* Keep only path */
    return path.substr(0, iPos);
}

/**
 * Cross-platform implementation of basename
 */
std::string getFilename ( std::string path ) {
    std::string ans = "";
    #ifndef _WIN32
    
        // Copy path to char * because dirname might modify it
        char * srcDirname = (char *) malloc( path.length() + 1 );
        path.copy( srcDirname, path.length(), 0 );
        srcDirname[ path.length() ] = '\0';
        
        // Do the conversion
        char * dirname = ::basename( srcDirname );
        ans = dirname;
        
        // Release pointer
        free( srcDirname );
        
    #else
    
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath_s(
                path.c_str(),
                NULL, 0,
                NULL, 0,
                fname, _MAX_FNAME,
                ext, _MAX_EXT
            );
        ans += fname;
        ans += ext;
        
    #endif
    return ans;
}

/**
 * Run prepareAppDataPath only once, then use string singleton
 */
std::string getAppDataPath() {
    if (appDataDir.empty())
        appDataDir = prepareAppDataPath();
    return appDataDir;
}

/**
 * Split the given line into a key and value using the delimited provided
 */
int getKV( string line, string * key, string * value, unsigned char delim, int offset ) {
    size_t a = line.find( delim, offset );
    if (a == string::npos) return 0;
    *key = line.substr(offset, a-offset);
    size_t b = a+1;
    while ( (b<line.length()) && ((line[b] == ' ') || (line[b] == '\t')) ) b++;
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
int parseLines( std::vector< std::string > * lines, std::map< std::string, std::string > * map, std::string csplit, std::string ctrim, size_t key, size_t value ) {
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
    
    /* Ignore invalid output buffer */
    if (out == NULL) return;

    /* Pass output into an input stream */
    istringstream ss( rawString );

    /* Split new lines and store them in the vector */
    size_t iTrim;
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
 * Split the given string into a vector of strings using white space as delimiter, while preserving
 * strin contents found in double quotes.
 */
int splitArguments( std::string source, char ** charBuffer, int bufferSize, int bufferOffset ) {
    static vector<string> args;
    size_t wsPos=0, sqPos=0, dqPos=0, qPos=0, iPos=0;
    string chunk; char nextChar = ' ';

    // Clear static args array
    args.clear();

    // Start analyzer
    while (iPos < source.length()) {
        wsPos = source.find(' ', iPos);
        sqPos = source.find('"', iPos);
        dqPos = source.find('\'', iPos);

        // Nothing found? We ran out of string
        if ((wsPos == string::npos) && (sqPos == string::npos) && (dqPos == string::npos)) {

            // Stack all the remains
            chunk = source.substr( iPos, source.length() - iPos );
            args.push_back( chunk );
            break;
        
        // State 1: Checking for whitespace
        } else if ((nextChar == ' ') && (wsPos < sqPos) && (wsPos < sqPos)) {
            chunk = source.substr( iPos, wsPos - iPos );
            args.push_back( chunk );
            iPos = wsPos+1;

        // State 2: Starting quote
        } else if ((nextChar == ' ') && (
            ( (wsPos == string::npos) && (sqPos != string::npos) ) || /* Only single quote found */
            ( (wsPos == string::npos) && (dqPos != string::npos) ) || /* Only double quote found */
            ( (wsPos > sqPos ) || (wsPos > dqPos ) )                  /* Or whitespace found after a quote */
            )) {
            
            // Switch to quote capture mode
            if ((dqPos == string::npos) || (dqPos > sqPos)) {
                nextChar = '\''; /* Only single quote found, or it's before the double */
                qPos = sqPos;
            } else {
                nextChar = '"'; /* Any other case is double quote */
                qPos = dqPos;
            }

            // Mark beginning
            iPos = qPos+1;

        // State 3a: Looking for single quote only (')
        } else if ((nextChar == '\'') && (sqPos != string::npos)) {

            // Stack the string contents
            chunk = source.substr( iPos, sqPos - iPos );
            args.push_back( chunk );
            iPos = sqPos+1;

            // Exit quote maching mode
            nextChar = ' ';

        // State 3b: Looking for double quote only (")
        } else if ((nextChar == '"') && (dqPos != string::npos)) {

            // Stack the string contents
            chunk = source.substr( iPos, dqPos - iPos );
            args.push_back( chunk );
            iPos = dqPos+1;

            // Exit quote maching mode
            nextChar = ' ';

        } else {

            // We should NEVER reach this point
            CVMWA_LOG("Error", "Unhandled case");

        }

    }

    // Cast back to the charBuffer
    int i = bufferOffset;
    for (vector<string>::iterator it = args.begin(); it < args.end(); it++) {

        // Check if we filled the buffer
        if ((i+1) >= bufferSize) {
            charBuffer[i] = (char*)NULL;
            break;
        }

        /* CHECK: My guess is that since 'args' is static, the
                  pointer to the buffer within the string will 
                  be preserved even when the function exit. 
                  Is it true? */
        charBuffer[i++] = (char *)(*it).c_str();

    }

    // Put a null char on the end
    charBuffer[i] = (char*)NULL;

    // Return how many components were placed in the charBuffer
    return i;

}

/**
 * Tokenize a key-value like output from VBoxManage into an easy-to-use hashmap
 */
map<string, string> tokenize( vector<string> * lines, char delim ) {
    map<string, string> ans;
    string line, key, value;
    if (lines->empty()) return ans;
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
 * Cross-platform function to return the temporary folder name
 */
std::string getTmpDir() {
#ifdef _WIN32
    char tmpPath[MAX_PATH];
    DWORD wRet = GetTempPathA( MAX_PATH, tmpPath );
    if (wRet == 0) return "";
    std::string path = tmpPath;
    if (path[path.length() - 1] == '\\')
        return path.substr(0,path.length()-1);
    return path;
#else
    return "/tmp";
#endif
}

/**
 * Return a temporary filename
 */
std::string getTmpFile( string suffix, string folder ) {
    #ifdef _WIN32
        DWORD wRet;
        int lRet;
        char tmpPath[MAX_PATH];
        char tmpName[MAX_PATH];

        /* Prohibit too long folder names */
        if (folder.length() + suffix.length() + 1 > MAX_PATH) return "";
        
        /* Get temporary folder, if it's not specified */
        if (folder.empty()) {
            wRet = GetTempPathA( MAX_PATH, tmpPath );
            if (wRet == 0) return "";
        } else {
            folder.copy( tmpPath, folder.length() );
            tmpPath[ folder.length() ] = '\0';
        }

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

        /* If no folder is specified, use /tmp */
        if (folder.empty())
            folder = "/tmp";

        /* Append template */
        folder += "/tmpXXXXXX";

        /* Copy to pointer */
        char * fName = new char[ folder.length()+1 ];
        folder.copy( fName, folder.length(), 0);
        fName[ folder.length() ] = '\0';

        /* Make unique name */
        int i = mkstemp(fName);
        if (i < 0) {
            delete[] fName;
            return "";
        }

        /* Close and cleanup file */
        close(i);
        remove( fName );

        /* Copy to std::string and cleanup */
        string tmpFile( fName );
        tmpFile += "." + suffix;
        delete[] fName;

        /* Return std::string version of the filename */
        return tmpFile;

    #endif
}

/**
 * Global variables shared with abortSysExec() and sysExec()
 */
#ifndef _WIN32
bool    sysExecAborted = false;
#else
HANDLE  sysExecAbortMutex = NULL;
#endif

/**
 * Global trigger to abort execution
 */
void abortSysExec() {
#ifndef _WIN32
    sysExecAborted = true;
#else
    if (sysExecAbortMutex != NULL)
        ReleaseMutex( sysExecAbortMutex );
#endif
}

/**
 * Cross-platform exec and return function (called by sysExec())
 */
int __sysExec( string app, string cmdline, vector<string> * stdoutList, string * rawStderr ) {
#ifndef _WIN32
    
    int ret = 253;
    pid_t pidChild;
    string item;
    string rawStdout = "";

    /* Prepare the two pipes */
    int outfd[2]; if (pipe(outfd) < 0) return HVE_IO_ERROR;
    int errfd[2]; if (pipe(errfd) < 0) return HVE_IO_ERROR;

    int oldstdout = dup(1); // Save current stdout
    if (oldstdout < 0) return HVE_IO_ERROR;
    int oldstderr = dup(2); // Save current stderr
    if (oldstderr < 0) {
        close(oldstdout);
        return HVE_IO_ERROR;
    }

    close(1); dup2(outfd[1],1); // Make the write end of outfd pipe as stdout
    close(2); dup2(errfd[1],2); // Make the write end of errfd pipe as stderr

    /* Fork to create child instance */
    pidChild = fork();
    if (pidChild == -1) {

        /* Return error code if something went wrong */
        return 254;

    } else if(!pidChild) {
        
        /* Pipes are not required for the child */
        close(outfd[0]); close(errfd[0]); 
        close(outfd[1]); close(errfd[1]);

        /* Close other handles */
        close(oldstdout);
        close(oldstderr);

        /* Split cmdline into string components */
        char *parts[512];
        parts[0] = (char *)app.c_str();
        splitArguments( cmdline, parts, 512, 1 );
        
        /* Launch given process */
        execv(app.c_str(), parts);

        /* We reach this point if execv fails */
        return 254;

    } else {

        char data[1035];
        ssize_t dataLen;

        /* Restore the original std fds of parent */
        close(1); dup2(oldstdout, 1); close(oldstdout);
        close(2); dup2(oldstderr, 2); close(oldstderr);

        /* These are being used by the child */
        close(outfd[1]); close(errfd[1]);

        /* Prepare the poll fd list */
        struct pollfd fds[2];
        fds[0].fd = outfd[0]; fds[0].events = POLLIN;
        fds[1].fd = errfd[0]; fds[1].events = POLLIN;

        /* Start reading stdin/err */
        long startTime = getMillis();
        for (;;) {

            /* Poll descriptors */
            ret = poll(fds, 2, 100);
            if (ret > 0) {

                /* An event on one of the fds has occurred. */
                for (int i=0; i<2; i++) {
                    if (fds[i].revents & POLLIN) {
                        // Data is available on fds[i]
                        dataLen = read(fds[i].fd, data, sizeof(data)-1);
                        if (dataLen > 0) {
                            if (i == 0) {
                                rawStdout.append(data, dataLen);
                            } else {
                                rawStderr->append(data, dataLen);
                            }
                        } else {
                            // Error while reading
                            ret = -1;
                        }
                    }
                    if (fds[i].revents & POLLHUP) {
                        // pipe hung-up.
                        // Set ret to -1 to flag termination
                        ret = -1;
                    }
                }
            }

            /* Abort if it takes way too long */
            if ( sysExecAborted || ((getMillis() - startTime) > SYSEXEC_TIMEOUT) ) {
                CVMWA_LOG("Debug", "Aborting execution");

                // Kill process
                kill( pidChild, SIGKILL );

                // Abort execution
                *rawStderr = "ERROR: Aborted";
                return 255;

            }

            /* Debug log stderror */
#if defined(DEBUG) || defined(LOGGING)
            if (!rawStderr->empty()) {
                CVMWA_LOG("Debug", "STDERR:" << *rawStderr);
            }
#endif

            /* Exit on error */
            if (ret < 0) 
                break;

        }

        /* Split stdout lines */
        splitLines( rawStdout, stdoutList );

        /* Wait forked pid to exit */
        waitpid(pidChild, &ret, 0);

        /* Otherwise, return the error code */
        return ret;
    }

     /* Should not reach this point, but what the heck */
     return ret;
#else
	HANDLE g_hChildStdOut_Rd = NULL;
	HANDLE g_hChildStdOut_Wr = NULL;
	HANDLE g_hChildStdErr_Rd = NULL;
	HANDLE g_hChildStdErr_Wr = NULL;
	DWORD ret = 253, dwRead, dwAvailable;
	CHAR chBuf[4096];
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	BOOL bSuccess = FALSE;
	string rawStdout = "";

	SECURITY_ATTRIBUTES sAttr;
	sAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
	sAttr.bInheritHandle = TRUE;
	sAttr.lpSecurityDescriptor = NULL;

    /* On the first time we call sysExec(), setup the mutex to be
       used on abortSysExec() */
    if (sysExecAbortMutex == NULL)
        sysExecAbortMutex = CreateMutex(NULL,FALSE,NULL);

    /* Create STDOUT pipe */
	if (!CreatePipe(&g_hChildStdOut_Rd, &g_hChildStdOut_Wr, &sAttr, 0)) 
		return HVE_IO_ERROR;
	if (!SetHandleInformation(g_hChildStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
		return HVE_IO_ERROR;

    /* Create STDERR pipe */
	if (!CreatePipe(&g_hChildStdErr_Rd, &g_hChildStdErr_Wr, &sAttr, 0)) 
		return HVE_IO_ERROR;
	if (!SetHandleInformation(g_hChildStdErr_Rd, HANDLE_FLAG_INHERIT, 0))
		return HVE_IO_ERROR;

    /* Clean structures */
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory( &siStartInfo, sizeof( STARTUPINFOA ));

    /* Prepare startup information */
	siStartInfo.cb = sizeof(STARTUPINFOA);
	siStartInfo.hStdOutput = g_hChildStdOut_Wr;
    siStartInfo.hStdError = g_hChildStdErr_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    /* Build cmdline */
    string execpath = "\"" + app + "\" " + cmdline;

    /* Create process */
    CVMWA_LOG("Debug", "Exec CMDLINE: " << cmdline);
	if (!CreateProcessA(
		NULL,
		(LPSTR)execpath.c_str(),
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
	CloseHandle( g_hChildStdErr_Wr );
	if ( stdoutList != NULL ) {
	    
	    /* Read to buffers */
    	for (;;) {

            /* Check for STDERR data (Never break on errors here) */
            if (PeekNamedPipe( g_hChildStdErr_Rd, NULL, NULL, NULL, &dwAvailable, NULL)) {
                if (dwAvailable > 0) {
    		        bSuccess = ReadFile( g_hChildStdErr_Rd, chBuf, 4096, &dwRead, NULL);
    		        if ( bSuccess && (dwRead > 0) ) {
                        rawStderr->append( chBuf, dwRead );
                    }
                }
            }

            /* Check for STDOUT data */
            if (!PeekNamedPipe( g_hChildStdOut_Rd, NULL, NULL, NULL, &dwAvailable, NULL)) break;
            if (dwAvailable > 0) {
    		    bSuccess = ReadFile( g_hChildStdOut_Rd, chBuf, 4096, &dwRead, NULL);
    		    if ( !bSuccess || dwRead == 0 ) break;
    		    rawStdout.append( chBuf, dwRead );
            }

            /* Sleep a teensy bit not to stress the CPU on the
             * infinite loop we are currently in */
            Sleep( 10 );

        }

        /* Debug STDERR */
        if (!rawStderr->empty())
            CVMWA_LOG("Debug", "Exec STDERR: " << *rawStderr);

	    /* Split lines into stdout */
        splitLines( rawStdout, stdoutList );
        
	}
	CloseHandle( g_hChildStdOut_Rd );
	CloseHandle( g_hChildStdErr_Rd );

    /* Wait for completion */
    const HANDLE handles[] = { sysExecAbortMutex, piProcInfo.hProcess }; 
	DWORD ans = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );

    /* Check for aborted */
    if ((ans - WAIT_OBJECT_0) == 0) {
        CVMWA_LOG("Debug", "Exec aborted");
        return 255;
    }

    /* Process exited successfully */
	GetExitCodeProcess( piProcInfo.hProcess, &ret );

    /* Close hanles */
	CloseHandle( piProcInfo.hProcess );
	CloseHandle( piProcInfo.hThread );

    /* Return exit code */
	return ret;
#endif
}

/**
 * Cross-platform exec function with retry functionality
 */
int sysExec( string app, string cmdline, vector<string> * stdoutList, string * rawStderrAns, int retries ) {
    string stdError;
    int res = 253;

    // Start the retry loop
    for (int tries = 0; tries < retries; tries++ ) {
        
        // Call the wrapper function
        res = __sysExec( app, cmdline, stdoutList, &stdError );
        CVMWA_LOG("Debug", "Exec EXIT_CODE: " << res);

        // Check for "Error" in the stderr
        // (Caused by a weird bug on VirtualBox)
        if ((stdError.find("error") != string::npos) || (stdError.find("ERROR") != string::npos) || (stdError.find("Error") != string::npos)) {
            if (rawStderrAns != NULL) *rawStderrAns = stdError;
            return 254;
        }

        // If it was successful, or we were aborted, return now. No retries.
        if ((res == 0) || (res == 255)) {
            break;
        } else {
            // Wait and retry
            CVMWA_LOG( "Info", "Going to retry in 1s. Try " << (tries+1) << "/" << retries  );
            sleepMs( 1000 );
        }
    }

    return res;
}

/**
 * Compare two paths for eqality (ignoring different kinds of slashes)
 */
bool samePath( std::string pathA, std::string pathB ) {
    for (size_t i = 0; i < pathA.length(); i++) {
        if (i >= pathB.length()) return false;
        char a = pathA[i], b = pathB[i];
        if (a == '\\') a = '/';
        if (b == '\\') b = '/';
        if (a != b) return false;
    }
    return true;
}

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
 * OpenSSL SHA256 on file 
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
 * OpenSSL SHA256 on string buffer
 */
int sha256_buffer( string buffer, string * checksum ) {
    char outputBuffer[65];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer.c_str(), buffer.length());
    SHA256_Final(hash, &sha256);
    __sha256_hash_string( hash, outputBuffer);
    *checksum = outputBuffer;
    
    return 0;
}

/**
 * OpenSSL SHA256 on string buffer
 */
int sha256_bin( string buffer, unsigned char * hash ) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer.c_str(), buffer.length());
    SHA256_Final(hash, &sha256);
    return 0;
}
/**
 * Base64-encoding borrowed from:
 * http://stackoverflow.com/questions/5288076/doing-base64-encoding-and-decoding-in-openssl-c
 */
::std::string base64_encode( const ::std::string &bindata ) {
   using ::std::string;
   using ::std::numeric_limits;

   /*
   if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
      throw ::std::length_error("Converting too large a string to base64.");
   }
   */

   const ::std::size_t binlen = bindata.size();
   // Use = signs so the end is properly padded.
   string retval((((binlen + 2) / 3) * 4), '=');
   ::std::size_t outpos = 0;
   int bits_collected = 0;
   unsigned int accumulator = 0;
   const string::const_iterator binend = bindata.end();

   for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
      accumulator = (accumulator << 8) | (*i & 0xffu);
      bits_collected += 8;
      while (bits_collected >= 6) {
         bits_collected -= 6;
         retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
      }
   }
   if (bits_collected > 0) { // Any trailing bits that are missing.
      assert(bits_collected < 6);
      accumulator <<= 6 - bits_collected;
      retval[outpos++] = b64_table[accumulator & 0x3fu];
   }
   assert(outpos >= (retval.size() - 2));
   assert(outpos <= retval.size());
   return retval;
}

/**
 * Base64-encoding borrowed from:
 * http://stackoverflow.com/questions/5288076/doing-base64-encoding-and-decoding-in-openssl-c
 */
::std::string base64_decode(const ::std::string &ascdata)
{
   using ::std::string;
   string retval;
   const string::const_iterator last = ascdata.end();
   int bits_collected = 0;
   unsigned int accumulator = 0;

   for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
      const int c = *i;
      if (::std::isspace(c) || c == '=') {
         // Skip whitespace and padding. Be liberal in what you accept.
         continue;
      }
      if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
         throw ::std::invalid_argument("This contains characters not legal in a base64 encoded string.");
      }
      accumulator = (accumulator << 6) | reverse_table[c];
      bits_collected += 6;
      if (bits_collected >= 8) {
         bits_collected -= 8;
         retval += (char)((accumulator >> bits_collected) & 0xffu);
      }
   }
   return retval;
}

/**
 * Check if the given port is open
 */
bool isPortOpen( const char * host, int port, unsigned char handshake ) {
    SOCKET sock;

    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_family = AF_INET;
    client.sin_port = htons( port );
    client.sin_addr.s_addr = inet_addr( host );
    
    // Open a connection
    sock = (SOCKET) socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    int result = connect(sock, (struct sockaddr *) &client,sizeof(client));

    // Check for open failures
    if (result < 0) {
        #ifdef _WIN32
            closesocket(sock);
        #else
            ::close(sock);
        #endif
        return false;
    }

    // If we have simple handsake, do it now
    if (handshake == HSK_SIMPLE) {

        // Try to send some data
        int n = send(sock," \n",2,0);
        if (n < 0) {
            #ifdef _WIN32
                closesocket(sock);
            #else
                ::close(sock);
            #endif
            return false;
        }

        // Check if it's still connected
        int errorCode = 0;
        socklen_t szErrorCode = sizeof(errorCode);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*) &errorCode, &szErrorCode );
        if (errorCode != 0 ){
            #ifdef _WIN32
                closesocket(sock);
            #else
                ::close(sock);
            #endif
            return false;
        }

    // If we have HTTP handsake, do it now
    } else if (handshake == HSK_HTTP) {

        // Prepare the request
        std::ostringstream oss;
        oss << "GET / HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Connection: close\r\n"
            << "\r\n";

        // Send it and check for failures
        int n = send(sock, oss.str().c_str() , oss.str().length(), 0);
        if (n < 0) {
            #ifdef _WIN32
                closesocket(sock);
            #else
                ::close(sock);
            #endif
            return false;
        }

        // Try to read a chunk
        char readBuf[1024];
        n = recv(sock, readBuf, 1024, 0);
        if (n <= 0) {

            // Either an error, or connection closed without
            // any data sent. However we did provide a valid HTTP 
            // request, which means the remote endpoint is not really
            // an HTTP server, or something went really wrong.
            #ifdef _WIN32
                closesocket(sock);
            #else
                ::close(sock);
            #endif
            return false;

        }

    }

    // It works
    #ifdef _WIN32
        closesocket(sock);
    #else
        ::close(sock);
    #endif
    return true;

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
bool isSanitized( std::string * check, const char * chars ) {
    int numChars = strlen(chars); /* < 'chars' is always provided by the developer */
    bool foundInChars = false;
    for (size_t j=0; j<check->length(); j++) {
        foundInChars = false;
        char c = check->at(j);
        for (int i=0; i<numChars; i++) {
            if (c == chars[i]) {
                foundInChars = true;
                break;
            }
        }
        if (!foundInChars) return false;
    }
    return true;
}

/**
 * Decompress a GZipped file from src and write it to dst
 */
int decompressFile( const std::string& src, const std::string& dst ) {
    
    // Try to open gzfile
    gzFile file;
    file = gzopen ( src.c_str(), "rb" );
    if (!file) {
        CVMWA_LOG("Error", "Unable to open GZ-Compressed file " << src);
        return HVE_NOT_FOUND;
    }
    std::ofstream  out( dst.c_str(), std::ofstream::binary );
    if ( ! out.good()) {
        CVMWA_LOG("Error", "Unable to open file file `" << dst << "' for writing.");
	    return HVE_IO_ERROR;
    }
    
    // Tune buffer for input speed
    gzbuffer( file, GZ_BLOCK_SIZE );
    
    // Decompress
    int bytes_written = 0;
    while (1) {
        int err;                    
        int bytes_read;
        unsigned char buffer[GZ_BLOCK_SIZE];
        
        // Read block
        bytes_read = gzread (file, buffer, GZ_BLOCK_SIZE - 1);
        if (bytes_read > 0) bytes_written+=bytes_read;
        
        // Write block
        out.write( (const char *) buffer, bytes_read );
        
        // Check for error/completion
        if (bytes_read < GZ_BLOCK_SIZE - 1) {
            if (gzeof(file)) {
                // File is completed
                break;
                
            } else {
                // Handle errors
                const char * error_string = gzerror(file, &err);
                if (err) {
                    CVMWA_LOG("Error", "GZError '" << error_string << "'");
                    return HVE_IO_ERROR;
                }
                
            }
        }
    }
    
    // Close streams
    out.close();
    gzclose(file);
    
    // If we did not read something, the file
    // was not in GZ-format
    if (bytes_written == 0) {
        return HVE_NOT_SUPPORTED;
    } else {
        return HVE_OK;
    }
    
}


/**
 * Encode the given string for URL
 */
std::string urlEncode ( const std::string &s ) {

    //RFC 3986 section 2.3 Unreserved Characters (January 2005)
    const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

    std::string escaped="";
    for(size_t i=0; i<s.length(); i++)
    {
        if (unreserved.find_first_of(s[i]) != std::string::npos)
        {
            escaped.push_back(s[i]);
        }
        else
        {
            escaped.append("%");
            char buf[3];
            sprintf(buf, "%.2X", (unsigned char) s[i]);
            escaped.append(buf);
        }
    }
    return escaped;
}

/**
 * Explode the given string on the given character as separator
 */
void explode( std::string const &input, char sep, std::vector<std::string> * output ) {
    std::istringstream buffer(input);
    std::string temp;
    while (std::getline(buffer, temp, sep))
        output->push_back(temp);
}

// Initialize template

#ifdef __linux__

/**
 * Identify the linux environment and return it's id
 * (Used by the hypervisor installation system in order to pick the appropriate binary)
 */
void getLinuxInfo ( LINUX_INFO * info ) {
    std::vector< std::string > vLines;
    std::string stdError;
    
    // Check if we have gksudo
    info->hasGKSudo = file_exists("/usr/bin/gksudo");
    info->hasPKExec = file_exists("/usr/bin/pkexec");
    info->hasXDGOpen = file_exists("/usr/bin/xdg-open");
    
    // Identify pakage manager
    info->osPackageManager = PMAN_NONE;
    if (file_exists("/usr/bin/dpkg")) {
        info->osPackageManager = PMAN_DPKG;
    } else if (file_exists("/usr/bin/yum")) {
        info->osPackageManager = PMAN_YUM;
    }
    
    // Use lsb_release to identify the linux version
    info->osDistID = "generic";
    if (file_exists("/usr/bin/lsb_release")) {
        
        // First, get release
        if (sysExec( "/usr/bin/lsb_release", "-i -s", &vLines, &stdError ) == 0) {
            if (vLines[0].compare("n/a") != 0) {
                info->osDistID = vLines[0];
            }
        }
        
        // Then get codename
        if (sysExec( "/usr/bin/lsb_release", "-c -s", &vLines, &stdError ) == 0) {
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
