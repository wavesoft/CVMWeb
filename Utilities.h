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

#ifndef UTILITIES_H_JA64LPSF
#define UTILITIES_H_JA64LPSF

/**
 * Global includes
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>

// Only for WIN32
#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#endif

// Common on *NIX platforms
#ifndef _WIN32
#define SOCKET          int
#include <fcntl.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <libgen.h>
#endif

// only for linux
#ifdef __linux__
#endif

// Only for apple
#if defined(__APPLE__) && defined(__MACH__)
#endif

/**
 * Progress feedback structure
 */
typedef struct {
    
    int                 min;
    int                 max;
    int                 total;
    void *              data;
    std::string         message;
    long int            lastEventTime;
    void (*callback)    (int, int, std::string, void *);
    
} HVPROGRESS_FEEDBACK;

/**
 * Get the location of the application's AppData folder
 */
std::string                                         getAppDataPath  ( );

/**
 * Remove a trailing folder from the given path
 */
std::string                                         stripComponent  ( std::string path );

/**
 * Cross-platform basename implementation
 */
std::string                                         getFilename     ( std::string path );

/**
 * Download a string from the given URL
 */
int                                                 downloadText    ( std::string url, std::string * buffer );

/**
 * Download a file from the given URL to the given target
 */
int                                                 downloadFile    ( std::string url, std::string target, HVPROGRESS_FEEDBACK * fb );

/**
 * Get the base64 representation of the given string buffer
 */
std::string                                         base64_encode   ( const ::std::string &bindata );

/**
 * Get the binary contents of the base64-encoded string
 */
std::string                                         base64_decode   ( const ::std::string &ascdata );

/**
 * Get the sha256 signature of the given path and store it on the
 * string in the checksum pointer
 */
int                                                 sha256_file     ( std::string path, std::string * checksum );

/**
 * Get the sha256 signature of the given buffer and store it on the
 * string in the checksum pointer
 */
int                                                 sha256_buffer   ( std::string path, std::string * checksum );

/**
 * Get the sha256 signature of the given buffer and store it on the checksum char pointer
 */
int                                                 sha256_bin   ( std::string path, unsigned char * checksum );

/**
 * Platform-independant function to execute the given command-line and return the
 * STDOUT lines to the string vector in *stdoutList.
 *
 * stdoutList can be NULL if you are not interested in collecting the output.
 * 
 * This function will wait for the command to finish and it will return it's exit code
 */
int                                                 sysExec         ( std::string cmdline, std::vector<std::string> * stdoutList );

/**
 * Return the name of a system-temporary file, appending the given suffix to it.
 */
std::string                                         getTmpFile      ( std::string suffix );

/**
 * Convert a hexadecimal int, short or long from string to it's numeric representation
 */
template <typename T> T                             hex_ston        ( const std::string &Text );

/**
 * Convert a decimal int, short or long from string to it's numeric representation
 */
template <typename T> T                             ston            ( const std::string &Text );

 /**
  * Convert a numeric value to it's string representation
  */
template <typename T> std::string                   ntos            ( T &value );

/**
 * Split the given *line into *parts, using the characters found in the split string as delimiters
 * while removing the trailing characters found in the trim string.
 */
int                                                 trimSplit       ( std::string * line, std::vector< std::string > * parts, std::string split, std::string trim );

/**
 * Apply the trimSplit() function for every line in *lines. Then create a map using the splitted
 * element at position <key> as key and the element at position <value> as value.
 */
int                                                 parseLines       ( std::vector< std::string > * lines, std::map< std::string, std::string > * map, std::string csplit, std::string ctrim, size_t key, size_t value );

/**
 * Check if the specified port is accepting connections
 */
bool                                                isPortOpen      ( const char * host, int port );

/**
 * Visualize the dump of a string:string hash map
 */
void                                                mapDump         ( std::map< std::string, std::string> m );

/**
 * Display a hexdump of the given buffer
 */
void                                                hexDump         (const char *desc, void *addr, int len);

/**
 * Allow only the given characters on the string specified
 */
bool                                                isSanitized     ( std::string * check, const char * chars );

/**
 * Decompress a GZipped file from src and write it to dst
 */
int                                                 decompressFile  ( std::string filename, std::string output );

/* ======================================================== */
/*                  PLATFORM-SPECIFIC CODE                  */
/* ======================================================== */

#ifdef __linux__

/**
 * Constants for the osPackageManager
 */
#define  PMAN_NONE      0
#define  PMAN_YUM       1
#define  PMAN_DPKG      2

/**
* Linux platform identification information
*/
typedef struct {

    unsigned char       osPackageManager;       // Package manager found on the system
    std::string         osDistID;               // Platform identification
    bool                hasGKSudo;              // If we have graphical SUDO display
    bool                hasXDGOpen;

} LINUX_INFO;

/**
 * Identify the linux environment and return it's id
 * (Used by the hypervisor installation system in order to pick the appropriate binary)
 */
void                                                getLinuxInfo    ( LINUX_INFO * info );
    
#endif

/**
 * OLD TOKENIZATION FUNCTIONS THAT SHOULD BE REMOVED
 */
std::vector< std::map<std::string, std::string> >   tokenizeList    ( std::vector<std::string> * lines, char delim );
std::map<std::string, std::string>                  tokenize        ( std::vector<std::string> * lines, char delim );
void                                                splitLines      ( std::string rawString, std::vector<std::string> * out );
int                                                 getKV           ( std::string line, std::string * key, std::string * value, unsigned char delim, int offset );

/* ======================================================== */
/*                    INLINE FUNCTIONS                      */
/* ======================================================== */

/**
 * Returns true if the given file exists and is readable
 */
inline bool file_exists( std::string path_to_file )
 { return std::ifstream( (const char*)path_to_file.c_str()) ; }

/**
 * Returns the current time in milliseconds
 * Used for calibration / delays
 * (WARNING! This is not always the real time in ms! On windows it's the time since
 *  the system booted.)
 */
inline long getMillis() {
    #ifndef _WIN32
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    #else
    return GetTickCount() / 1000;
    #endif
}

/**
 * Debug macro
 */
#if defined(DEBUG) || defined(LOGGING)
#define CVMWA_LOG(kind, ...) \
     do { std::cerr << "[" << kind << "@" << __func__ << "] " << __VA_ARGS__ << std::endl; } while(0);
#else
#define CVMWA_LOG(...) ;
#endif

#endif /* end of include guard: UTILITIES_H_JA64LPSF */
