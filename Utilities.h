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

#include <curl/curl.h>
#include <curl/easy.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#endif

#ifndef _WIN32
#define SOCKET          int
#include <fcntl.h>
#include <sys/time.h>
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
    void (*callback)    (int, int, std::string, void *);
    
} HVPROGRESS_FEEDBACK;

/**
 * Download a string from the given URL
 */
int                                                 downloadText    ( std::string url, std::string * buffer );

/**
 * Download a file from the given URL to the given target
 */
int                                                 downloadFile    ( std::string url, std::string target, HVPROGRESS_FEEDBACK * fb );

/**
 * Get the base64 representation of the given char buffer
 */
std::string                                         base64_encode   (char const* bytes_to_encode, unsigned int in_len);

/**
 * Get the sha256 signature of the given path and store it on the
 * string in the checksum pointer
 */
int                                                 sha256_file     ( std::string path, std::string * checksum );

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
 * Split the given *line into *parts, using the characters found in the split string as delimiters
 * while removing the trailing characters found in the trim string.
 */
int                                                 trimSplit       ( std::string * line, std::vector< std::string > * parts, std::string split, std::string trim );

/**
 * Apply the trimSplit() function for every line in *lines. Then create a map using the splitted
 * element at position <key> as key and the element at position <value> as value.
 */
int                                                 parseLine       ( std::vector< std::string > * lines, std::map< std::string, std::string > * map, std::string csplit, std::string ctrim, size_t key, size_t value );

/**
 * Check if the specified port is accepting connections
 */
bool                                                isPortOpen      ( const char * host, int port );

/**
 * Visualize the dump of a string:string hash map
 */
void                                                mapDump         ( std::map< std::string, std::string> m );

/**
 * OLD TOKENIZATION FUNCTIONS THAT SHOULD BE REMOVED
 */
std::vector< std::map<std::string, std::string> >   tokenizeList    ( std::vector<std::string> * lines, char delim );
std::map<std::string, std::string>                  tokenize        ( std::vector<std::string> * lines, char delim );
void                                                splitLines      ( std::string rawString, std::vector<std::string> * out );
int                                                 getKV           ( std::string line, std::string * key, std::string * value, unsigned char delim, int offset );


/**
 * Returns true if the given file exists and is readable
 */
inline bool file_exists( std::string path_to_file )
 { return std::ifstream( (const char*)path_to_file.c_str()) ; }

#endif /* end of include guard: UTILITIES_H_JA64LPSF */
