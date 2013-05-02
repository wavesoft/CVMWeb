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
 
#include <string.h>
#include "ThinIPC.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#endif

#if ( defined(__APPLE__) && defined(__MACH__) ) || defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#endif

using namespace std;

/* ***************************************************************************** */
/* *                           GLOBAL FUNCTIONS                                * */
/* ***************************************************************************** */

/**
 * Global socket initialization for use with Thin IPC
 */
int ThinIPCInitialize() {

    // Initialize Winsock on windows
    #ifdef _WIN32
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) return 1;
    #endif
    
    return 0;
}

/* ***************************************************************************** */
/* *                           MESSAGE DATA I/O                                * */
/* ***************************************************************************** */

// Static singleton for shorthands
ThinIPCMessage ThinIPCMessage::__shorthandInstance;

/**
 * Empty constructor
 */
ThinIPCMessage::ThinIPCMessage( ) {
    size = 0;
    this->data = NULL;
    __chunkPos = 0;
    __ioPos = 0;
};

/**
 * Initialize with data
 */
ThinIPCMessage::ThinIPCMessage( char* payload, short size ) {
    this->data = (char*) malloc( size );
    memcpy( this->data, payload, size );
    this->size = size;
    __chunkPos = 0;
    __ioPos = 0;
};

/**
 * Destructor
 */
ThinIPCMessage::~ThinIPCMessage( ) {
    if (this->data != NULL) free(this->data);
}

/**
 * Read arbitrary pointer object from stream
 */
template <typename T> short ThinIPCMessage::readPtr( T * ptr ) {
    int len = sizeof(T);
    if (len > 0xFFFF) return 0;
    if (__ioPos+len > size) return 0;
    // Read int
    memcpy( ptr, &this->data[__ioPos], len );
    __ioPos += len;
    return (short) len;
};

/**
 * Write arbitrary pointer object from stream
 */
template <typename T> short ThinIPCMessage::writePtr( T * ptr ) {
    int len = sizeof(T);
    if (len > 0xFFFF) return 0;
    if (__ioPos+len > size) return 0;
    // Read int
    memcpy( &this->data[__ioPos], ptr, len );
    __ioPos += len;
    return (short) len;
};

/**
 * Read integer from input stream
 */
int ThinIPCMessage::readInt() {
    int v;
    if (__ioPos+4 > size) return 0;

    // Read int
    memcpy( &v, &this->data[__ioPos], 4 );
    __ioPos += 4;
    
    return v;
};

/**
 * Read string from input stream
 */
string ThinIPCMessage::readString() {
    short len;
    if (__ioPos+2 > size) return "";
    
    // Read length
    memcpy( &len, &this->data[__ioPos], 2 );
    __ioPos += 2;
    
    // Read bytes
    if (__ioPos+len > size) return "";
    char * buf = (char *) malloc( len );
    memcpy( buf, &this->data[__ioPos], len );
    __ioPos += len;

    // Create response
    string v(buf, len);
    free( buf );
    return v;
};

/**
 * Write integer to output stream
 */
short ThinIPCMessage::writeInt( int v ) {
    // Resize buffer
    size += 4;
    this->data = (char *) realloc( this->data, size );
    
    // Write int
    memcpy( &this->data[__ioPos], &v, 4 );
    __ioPos += 4;
    
    // Return bytes written
    return 4;
};

/**
 * Write string to output stream
 */
short ThinIPCMessage::writeString( string v ) {
    // Resize buffer
    size += 2 + v.length();
    this->data = (char *) realloc( this->data, size );
    
    // Write length
    short len = (short) v.length();
    memcpy( &this->data[__ioPos], &len, 2 );
    __ioPos += 2;
    
    // Write string
    memcpy( &this->data[__ioPos], (char *)v.c_str(), len );
    __ioPos += len;
    
    // Return bytes written
    return len;
};

/**
 * Flush buffers and reset contents
 */
void ThinIPCMessage::reset() {
    if (this->data != NULL) free(this->data);
    this->data = NULL;
    size = 0;
    __ioPos = 0;
    __chunkPos = 0;
};

/**
 * Rewind position of read/write buffer
 */
void ThinIPCMessage::rewind() {
    __ioPos = 0;
    __chunkPos = 0;
}

/**
 * Extract data in chunks. 
 * This function will return the number of bytes copied on the output
 * buffer, or 0 when finished.
 */
short ThinIPCMessage::getChunk( char * buffer, short capacity ) {
    short cpSize;
    if (__chunkPos >= size) return 0;
    
    // If we are the first buffer, we are also going to put
    // the total size of the message as an integer in the beginning
    if ( __chunkPos == 0 ) {
        
        // Calculate copy chunk size
        cpSize = size - __chunkPos;
        if ((cpSize + 2) > capacity) cpSize = capacity-2;

        // Put size in the beginning
        memcpy( buffer, &size, 2 );
    
        // Perform copy, after the size
        memcpy( &buffer[2], &this->data[__chunkPos], cpSize );
        
        // Forward chunk position
        __chunkPos += cpSize;
        
        // Return the size of bytes copied
        return cpSize + 2;

    } else {
        
        // Calculate copy chunk size
        cpSize = size - __chunkPos;
        if (cpSize > capacity) cpSize = capacity;

        // Perform copy
        memcpy( buffer, &this->data[__chunkPos], cpSize );
        
        // Forward chunk position
        __chunkPos += cpSize;

    }
    
    // Return the size of bytes copied
    return cpSize;
};

/**
 * Read data in chunks. 
 * Calling this function will push data in the buffer. When completed, the
 * function will return 0. Otherwise it will return the number of bytes written.
 */
short ThinIPCMessage::putChunk( char * buffer, short capacity ) {
    if ( (__chunkPos == size) && (__chunkPos != 0) ) return 0;
    short readSize = capacity;
    
    // First chunk contains the message size
    if (__chunkPos == 0) {
        
        // Read chunk size (assuming capacity > 4)
        memcpy( &size, buffer, 2 );
        std::cout << "INFO: Read " << size << "\n";
        
        // Reset buffer
        if (this->data != NULL) free(this->data);
        this->data = (char *)malloc( size );
    
        // Calculate the number of bytes to read
        if ( __chunkPos + readSize > size ) readSize = size - __chunkPos;
        
        std::cout << "INFO: Read size " << readSize << " at " << __chunkPos << "\n";
        memcpy( &this->data[__chunkPos], &buffer[2], readSize-2 );
        __chunkPos += readSize;
        
    } else {
        
        // Calculate the number of bytes to read
        if ( __chunkPos + readSize > size ) readSize = size - __chunkPos;
        memcpy( &this->data[__chunkPos], buffer, readSize );
        __chunkPos += readSize;
        
    }
    
    // Return the number of the frame size when completed
    if ( __chunkPos >= size ) {
        return size;
    } else {
        return 0;
    }
}

/**
 * Shorthand : ( int )
 */
ThinIPCMessage * ThinIPCMessage::I( int v1 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    return &__shorthandInstance;
};

/**
 * Shorthand : ( string )
 */
ThinIPCMessage * ThinIPCMessage::S( string v1 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    return &__shorthandInstance;
};

/**
 * Shorthand : ( int, int )
 */
ThinIPCMessage * ThinIPCMessage::II( int v1, int v2 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    __shorthandInstance.writeInt( v2 );
    return &__shorthandInstance;
};

/**
 * Shorthand : ( string, string )
 */
ThinIPCMessage * ThinIPCMessage::SS( string v1, string v2 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    __shorthandInstance.writeString( v2 );
    return &__shorthandInstance;
};

/**
 * Shorthand : ( int, string )
 */
ThinIPCMessage * ThinIPCMessage::IS( int v1, string v2 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    __shorthandInstance.writeString( v2 );
    return &__shorthandInstance;
};

/**
 * Shorthand : ( string, int )
 */
ThinIPCMessage * ThinIPCMessage::SI( string v1, int v2 ) {
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    __shorthandInstance.writeInt( v2 );
    return &__shorthandInstance;
};

/* ***************************************************************************** */
/* *                     BASE SOCKET COMMUNICATION CLASS                       * */
/* ***************************************************************************** */

/**
 * Create a socket on the given port
 */
ThinIPCEndpoint::ThinIPCEndpoint( int port ) {
    
    // Create datagram socket
    this->errorCode = SCKE_OK;
    this->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if( this->sock < 0 ) {
        this->errorCode = SCKE_CREATE;
        return;
    }
    
    // Bind on localhost, on specified port
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind on the server address
    if( bind(this->sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
        this->errorCode = SCKE_BIND;
        return;
    }
    
}

/**
 * Cleanup
 */
ThinIPCEndpoint::~ThinIPCEndpoint() {
#ifndef _WIN32
    closeSocket(this->sock);
#else
    close(this->sock);
#endif
}

/**
 * Send data frame to the socket
 */
int ThinIPCEndpoint::send( int port, ThinIPCMessage * msg ) {
    char buf[1024];
    int len, clen, ctotal = 0;
    
    // Prepare the address to send to
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Start sending chunks
    msg->rewind();
    while ((clen = msg->getChunk( buf, 1024 )) > 0) {
        
        // Send frame
        socklen_t flen = sizeof(server);
        len = sendto( this->sock, buf, clen, 0, (struct sockaddr *)&server, flen );
        
        // Check errors
        if (len <= 0) return len;
        ctotal += len;
        
    }
    
    // Return bytes sent
    return ctotal;
    
}

/**
 * Wait for a data frame on the socket
 */
int ThinIPCEndpoint::recv( int port, ThinIPCMessage * msg ) {
    char buf[1024];
    int len;
    
    // Prepare the address to listen on
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Start receiving frame
    msg->reset();
    while (true) {
        
        // Fetch frame
        socklen_t flen = sizeof(server);
        len = recvfrom( this->sock, buf, 1024, 0, (struct sockaddr *)&server, &flen );
        if (len <= 0) return len;
        
        // Push on IPC frame
        len = msg->putChunk( buf, 1024 );
        if (len > 0) return len;
        
    }
    
}