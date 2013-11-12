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
#ifndef UTILITIES_H_JA64LPSF
#define UTILITIES_H_JA64LPSF

/**
 * Global includes
 */
#include <algorithm>
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

#include <sys/types.h>
#include <sys/stat.h>

// Constants for boost interprocess
#ifdef _WIN32
#define BOOST_INTERPROCESS_WINDOWS
#endif

// BOOST Subset of functions
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "CrashReport.h"

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
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <poll.h>
#include <unistd.h>
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

// String constants of EOL
#ifdef _WIN32
#define _EOL "\r\n"
#else
#define _EOL "\n"
#endif

// How much time (in ms) to wait for a command to return from sysExec( )
#define SYSEXEC_TIMEOUT     30000
#define SYSEXEC_SLEEP_DELAY 100
#define SYSEXEC_RETRY_DELAY 1000

// GZip decompression block size (64k)
#define GZ_BLOCK_SIZE 0x10000

/**
 * Mutex configuration for multi-threaded openssl
 */ 
#ifdef _WIN32
#define MUTEX_TYPE       HANDLE
#define MUTEX_SETUP(x)   x = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x) CloseHandle(x)
#define MUTEX_LOCK(x)    WaitForSingleObject(x, INFINITE)
#define MUTEX_UNLOCK(x)  ReleaseMutex(x)
#define THREAD_ID        GetCurrentThreadId(  )
#else
#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID        pthread_self(  )
#endif

/* isPortOpen Handshake Protocols */
#define HSK_NONE        0   // No port negotiation will happen
#define HSK_SIMPLE      1   // Just send a space and a newline and check if it's still connected
#define HSK_HTTP        2   // Send a basic HTTP GET / request and expect some data as response

/* Callback function definitions */
typedef boost::function< void () >                                                   callbackVoid;
typedef boost::function< void (const std::string&) >                                 callbackDebug;
typedef boost::function< void (const std::string&, const int, const std::string&) >  callbackError;
typedef boost::function<void ( const boost::shared_array<uint8_t>&, const size_t)>   callbackData;
typedef boost::function<void ( const size_t, const size_t, const std::string& )>     callbackProgress;

/**
 * Allocate a new GUID
 */
std::string                                         newGUID( );

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
int                                                 sha256_bin      ( std::string path, unsigned char * checksum );

/**
 * Get the md5 signature of the given buffer and store it on the checksum char pointer
 */
int                                                 md5_bin         ( std::string path, unsigned char * checksum );

/**
 * Platform-independant function to execute the given command-line and return the
 * STDOUT lines to the string vector in *stdoutList.
 *
 * stdoutList can be NULL if you are not interested in collecting the output.
 * 
 * This function will wait for the command to finish and it will return it's exit code
 */
int                                                 sysExec         ( std::string app, std::string cmdline, std::vector<std::string> * stdoutList, std::string * rawStderr, int retries = 1, int timeout = SYSEXEC_TIMEOUT );

/**
 * Platform-independant function to execute the given command-line without
 * waiting for it to complete.
 */
int                                                 sysExecAsync    ( std::string app, std::string cmdline );

/**
 * Initialize sysExec() environment
 */
void                                                initSysExec     ( );

/**
 * Abort an actively running sysExec() command
 */
void                                                abortSysExec    ( );

/**
 * Cross-platform function to return the temporary folder path
 */
std::string                                         getTmpDir       ( );

/**
 * Return the name of a system-temporary file, appending the given suffix to it.
 */
std::string                                         getTmpFile      ( std::string suffix, std::string folder = "" );

/**
 * Compare two paths for eqality (ignoring different kinds of slashes)
 */
bool                                                samePath        ( std::string pathA, std::string pathB );

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
 * Split the given string into a vector of strings using white space as delimiter, while preserving
 * strin contents found in double quotes.
 */
int                                                 splitArguments  ( std::string source, std::vector< std::string > * args );

/**
 * Check if the specified port is accepting connections
 */
bool                                                isPortOpen      ( const char * host, int port, unsigned char handshake = HSK_NONE );

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
int                                                 decompressFile  ( const std::string& filename, const std::string& output );

/**
 * Encode the given string for URL
 */
std::string                                         urlEncode       ( const std::string& url );

/**
 * Explode function
 */
void                                                explode         ( std::string const &input, char sep, std::vector<std::string> * output );

/**
 * Get current date/time as a string
 */
char *                                              getTimestamp    ();

/**
 * Generate Compact ID (30 characters) of the given id
 *
 * This function is used to get the names of the shared mutexes, since
 * the maximum character length is 30 and the session IDs can be much
 * longer.
 */
std::string                                         compactID       ( std::string id );

/**
 * Release memory from the named mutexes already acquired by the named mutex locks
 */
void                                                flushNamedMutexes ();

/**
 * Visualize the dump of a string:string hash map
 */
void                                                mapDump         ( std::map< std::string, std::string> m );

/**
 * Visualize the dump of a string:vector
 */
void                                                vectorDump      ( std::vector< std::string > v );


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
    bool                hasPKExec;              // Another GKSUDO approach
    bool                hasXDGOpen;

} LINUX_INFO;

/**
 * Identify the linux environment and return it's id
 * (Used by the hypervisor installation system in order to pick the appropriate binary)
 */
void                                                getLinuxInfo    ( LINUX_INFO * info );

/**
 * Utility function to check if the file is oppened by another process
 */
bool isFileOpen( string fileName );

/**
 * Wait for a file to be oppened within a specific time range
 */
bool waitFileOpen( string filename, bool forOpen, int waitMillis );

#endif

/**
 * OLD TOKENIZATION FUNCTIONS THAT SHOULD BE REMOVED
 */
std::vector< std::map<std::string, std::string> >   tokenizeList    ( std::vector<std::string> * lines, char delim );
std::map<std::string, std::string>                  tokenize        ( std::vector<std::string> * lines, char delim );
void                                                splitLines      ( std::string rawString, std::vector<std::string> * out );
int                                                 getKV           ( std::string line, std::string * key, std::string * value, unsigned char delim, int offset );

/* ======================================================== */
/*                INLINE FUNCTIONS & MACROS                 */
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
    return GetTickCount();
    #endif
}

/**
 * Cross-platform function to wait for some milliseconds
 */
inline void sleepMs(int sleepMs) {
    #ifdef _WIN32
    Sleep(sleepMs);
    #else
    usleep(sleepMs * 1000 );   // usleep takes sleep time in us
    #endif
}

/**
 * Named mutex context lock mechanisms
 */
typedef boost::shared_ptr< boost::mutex >   sharedMutex;
sharedMutex                                 __nmutex_get( std::string name );
#define NAMED_MUTEX_LOCK(x)                 { sharedMutex __mutex = __nmutex_get(x); boost::unique_lock<boost::mutex> __mLock( *__mutex.get() );
#define NAMED_MUTEX_UNLOCK                  sleepMs(500); }; 

/**
 * Convert to lowercase the given string
 */
#define toLowerCase(x)  std::transform( x.begin(), x.end(), x.begin(), ::tolower)

/** 
 * Macro to check if variant is null or missing
 */
#define IS_MISSING(x)       (x.empty() || x.is_of_type<FB::FBVoid>() || x.is_of_type<FB::FBNull>())
#define IS_CB_AVAILABLE(x)  (x.is_of_type<FB::JSObjectPtr>() && !IS_MISSING(x) )

/**
 * Debug macros
 */
#if defined(CRASH_REPORTING)
    #ifdef _WIN32
        #define CVMWA_REPORT_LOG(kind, ...) \
            do { std::ostringstream ss; ss << "[" << getTimestamp() << "][" << kind << "@" << __FUNCTION__ << "] " << __VA_ARGS__ << std::endl; crashReportStoreLog( ss ); } while(0);
    #else
        #define CVMWA_REPORT_LOG(kind, ...) \
            do { std::ostringstream ss; ss << "[" << getTimestamp() << "][" << kind << "@" << __func__ << "] " << __VA_ARGS__ << std::endl; crashReportStoreLog( ss ); } while(0);
    #endif
#else
    #define CVMWA_REPORT_LOG(...) ;
#endif
#if defined(DEBUG) || defined(LOGGING)
    #ifdef _WIN32
        #define CVMWA_LOG(kind, ...) \
            do { std::ostringstream ss; ss << "[" << getTimestamp() << "][" << kind << "@" << __FUNCTION__ << "] " << __VA_ARGS__ << std::endl; OutputDebugStringA( ss.str().c_str() ); } while(0); \
            CVMWA_REPORT_LOG(kind, __VA_ARGS__ )
    #else
        #define CVMWA_LOG(kind, ...) \
            do { std::cerr << "[" << getTimestamp() << "][" << kind << "@" << __func__ << "] " << __VA_ARGS__ << std::endl; } while(0); \
            CVMWA_REPORT_LOG(kind, __VA_ARGS__ )
    #endif
#else
    #define CVMWA_LOG(kind, ...) CVMWA_REPORT_LOG(kind, __VA_ARGS__)
#endif
#define WTF_LOG(sth) \
     std::cerr << "[WTF][" << __func__ << "](" << __FILE__ << ":" << __LINE__ << ") " << sth << std::endl;

#endif /* end of include guard: UTILITIES_H_JA64LPSF */
