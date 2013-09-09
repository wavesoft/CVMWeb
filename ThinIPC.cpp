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

using namespace std;

/* ***************************************************************************** */
/* *                           GLOBAL FUNCTIONS                                * */
/* ***************************************************************************** */

/**
 * Global socket initialization for use with Thin IPC
 */
int thinIPCInitialize() {
    CRASH_REPORT_BEGIN;

    // Initialize Winsock on windows
    #ifdef _WIN32
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) return 1;
    #endif
    
    // Seed random engine
    srand( getMillis() ); 
    return 0;
    CRASH_REPORT_END;
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
    CRASH_REPORT_BEGIN;
    size = 0;
    this->data = NULL;
    this->__chunkPos = 0;
    this->__ioPos = 0;

    CRASH_REPORT_END;
};

/**
 * Initialize with data
 */
ThinIPCMessage::ThinIPCMessage( char* payload, short size ) {
    CRASH_REPORT_BEGIN;

    this->__chunkPos = 0;
    this->__ioPos = 0;
    this->size = size;

    this->data = (char*) malloc( size );
    if (this->data == NULL) return;

    memcpy( this->data, payload, size );

    CRASH_REPORT_END;
};

/**
 * Destructor
 */
ThinIPCMessage::~ThinIPCMessage( ) {
    CRASH_REPORT_BEGIN;
    if (this->data != NULL) free(this->data);
    CRASH_REPORT_END;
}

/**
 * Read arbitrary pointer object from stream
 */
template <typename T> short ThinIPCMessage::readPtr( T * ptr ) {
    CRASH_REPORT_BEGIN;
    int len = sizeof(T);
    if (len > 0xFFFF) return 0;
    if (__ioPos+len > size) return 0;
    // Read int
    memcpy( ptr, &this->data[__ioPos], len );
    __ioPos += len;

    return (short) len;
    CRASH_REPORT_END;
};

/**
 * Write arbitrary pointer object from stream
 */
template <typename T> short ThinIPCMessage::writePtr( T * ptr ) {
    CRASH_REPORT_BEGIN;
    int len = sizeof(T);
    if (len > 0xFFFF) return 0;
    if (__ioPos+len > size) return 0;
    // Read int
    memcpy( &this->data[__ioPos], ptr, len );
    __ioPos += len;

    return (short) len;
    CRASH_REPORT_END;
};

/**
 * Read integer from input stream
 */
long int ThinIPCMessage::readInt() {
    CRASH_REPORT_BEGIN;
    long int v;
    if (__ioPos+4 > size) return 0;

    // Read int
    memset( &v, 0, 4 );
    memcpy( &v, &this->data[__ioPos], 4 );
    __ioPos += 4;

    return v;
    CRASH_REPORT_END;
};

/**
 * Read integer from input stream
 */
short int ThinIPCMessage::readShort() {
    CRASH_REPORT_BEGIN;
    short int v;
    
    if (__ioPos+2 > size) return 0;

    // Read int
    memset( &v, 0, 2 );
    memcpy( &v, &this->data[__ioPos], 2 );
    __ioPos += 2;

    return v;
    CRASH_REPORT_END;
};

/**
 * Read string from input stream
 */
string ThinIPCMessage::readString() {
    CRASH_REPORT_BEGIN;
    short len;
    if (__ioPos+2 > size) return "";
    
    // Read length
    memcpy( &len, &this->data[__ioPos], 2 );
    __ioPos += 2;
    
    // Read bytes
    if (__ioPos+len > size) return "";
    char * buf = (char *) malloc( len );
    if (buf == NULL) return "";
    memcpy( buf, &this->data[__ioPos], len );
    __ioPos += len;

    // Create response
    string v(buf, len);
    free( buf );

    return v;
    CRASH_REPORT_END;
};

/**
 * Write integer to output stream
 */
short ThinIPCMessage::writeInt( long int v ) {
    CRASH_REPORT_BEGIN;

    // Resize buffer
    size += 4;
    char * nData = (char *) realloc( this->data, size );

    // Check if there was an error to realloc
    // (and therefore the data pointer was not deallocated)
    if (nData == NULL) {
        size -= 4;
        return 0;
    } else {
        this->data = nData;
    }
    
    // Write int
    memcpy( &this->data[__ioPos], &v, 4 );
    __ioPos += 4;

    // Return bytes written
    return 4;
    CRASH_REPORT_END;
};

/**
 * Write short integer to output stream
 */
short ThinIPCMessage::writeShort( short int v ) {
    CRASH_REPORT_BEGIN;

    // Resize buffer
    size += 2;
    char * nData = (char *) realloc( this->data, size );
    
    // Check if there was an error to realloc
    // (and therefore the data pointer was not deallocated)
    if (nData == NULL) {
        size -= 4;
        return 0;
    } else {
        this->data = nData;
    }

    // Write int
    memcpy( &this->data[__ioPos], &v, 2 );
    __ioPos += 2;

    // Return bytes written
    return 2;
    CRASH_REPORT_END;
};

/**
 * Write string to output stream
 */
short ThinIPCMessage::writeString( string v ) {
    CRASH_REPORT_BEGIN;

    // Resize buffer
    size += 2 + v.length();
    char * nData = (char *) realloc( this->data, size );

    // Check if there was an error to realloc
    // (and therefore the data pointer was not deallocated)
    if (nData == NULL) {
        size -= 4;
        return 0;
    } else {
        this->data = nData;
    }

    // Write length
    short len = (short) v.length();
    memcpy( &this->data[__ioPos], &len, 2 );
    __ioPos += 2;
    
    // Write string
    memcpy( &this->data[__ioPos], (char *)v.c_str(), len );
    __ioPos += len;

    // Return bytes written
    return len;
    CRASH_REPORT_END;
};

/**
 * Flush buffers and reset contents
 */
void ThinIPCMessage::reset() {
    CRASH_REPORT_BEGIN;
    if (this->data != NULL) free(this->data);
    this->data = NULL;
    size = 0;
    __ioPos = 0;
    __chunkPos = 0;
    CRASH_REPORT_END;
};

/**
 * Rewind position of read/write buffer
 */
void ThinIPCMessage::rewind() {
    CRASH_REPORT_BEGIN;
    __ioPos = 0;
    __chunkPos = 0;
    CRASH_REPORT_END;
}

/**
 * Extract data in chunks. 
 * This function will return the number of bytes copied on the output
 * buffer, or 0 when finished.
 */
short ThinIPCMessage::getChunk( char * buffer, short capacity ) {
    CRASH_REPORT_BEGIN;
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
    CRASH_REPORT_END;
};

/**
 * Read data in chunks. 
 * Calling this function will push data in the buffer. When completed, the
 * function will return 0. Otherwise it will return the number of bytes written.
 */
short ThinIPCMessage::putChunk( char * buffer, short capacity ) {
    CRASH_REPORT_BEGIN;
    if ( (__chunkPos == size) && (__chunkPos != 0) ) return 0;
    short readSize = capacity;
    
    // First chunk contains the message size
    if (__chunkPos == 0) {
        
        // Read chunk size (assuming capacity > 2)
        memcpy( &size, buffer, 2 );
        
        // Reset buffer
        if (this->data != NULL) free(this->data);
        this->data = (char *)malloc( size );

        // Check for alloc failure
        if (this->data == NULL) {
            return 0;
        }
    
        // Check if the entire buffer fits in the chunk
        if ( size <= capacity) {
            memcpy( this->data, &buffer[2], size );
            return size;
        }
    
        // Otherwise write only current chunk
        __chunkPos = capacity-2;
        memcpy( this->data, &buffer[2], __chunkPos );
        
    } else {
        
        // Calculate data remaining
        readSize = size - __chunkPos;
        
        // Check if that's the last chunk
        if ( readSize <= capacity ) {
            memcpy( &this->data[__chunkPos], buffer, readSize );
            return size;
        }
        
        // Otherwise read only that chunk
        memcpy( &this->data[__chunkPos], buffer, capacity );
        __chunkPos += capacity;
        
    }
    
    // Still data left
    return 0;
    CRASH_REPORT_END;
}

/**
 * Shorthand : ( int )
 */
ThinIPCMessage * ThinIPCMessage::I( int v1 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/**
 * Shorthand : ( string )
 */
ThinIPCMessage * ThinIPCMessage::S( string v1 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/**
 * Shorthand : ( int, int )
 */
ThinIPCMessage * ThinIPCMessage::II( int v1, int v2 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    __shorthandInstance.writeInt( v2 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/**
 * Shorthand : ( string, string )
 */
ThinIPCMessage * ThinIPCMessage::SS( string v1, string v2 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    __shorthandInstance.writeString( v2 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/**
 * Shorthand : ( int, string )
 */
ThinIPCMessage * ThinIPCMessage::IS( int v1, string v2 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeInt( v1 );
    __shorthandInstance.writeString( v2 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/**
 * Shorthand : ( string, int )
 */
ThinIPCMessage * ThinIPCMessage::SI( string v1, int v2 ) {
    CRASH_REPORT_BEGIN;
    __shorthandInstance.reset();
    __shorthandInstance.writeString( v1 );
    __shorthandInstance.writeInt( v2 );
    return &__shorthandInstance;
    CRASH_REPORT_END;
};

/* ***************************************************************************** */
/* *                     BASE SOCKET COMMUNICATION CLASS                       * */
/* ***************************************************************************** */

/**
 * Create a socket on the given port
 */
ThinIPCEndpoint::ThinIPCEndpoint( int port ) {
    CRASH_REPORT_BEGIN;
    
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
    
//    std::cout << "INFO: ThinIPC(" << port << ")" << std::endl;
    
    // Bind on the server address
    if( ::bind(this->sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
        this->errorCode = SCKE_BIND;
        return;
    }
    
    CRASH_REPORT_END;
}

/**
 * Cleanup
 */
ThinIPCEndpoint::~ThinIPCEndpoint() {
    CRASH_REPORT_BEGIN;
//    std::cout << "INFO: ThinkIPC Cleanup" << std::endl;
#ifdef _WIN32
    closesocket(this->sock);
#else
    close(this->sock);
#endif
    CRASH_REPORT_END;
}

/**
 * Send data frame to the socket
 */
int ThinIPCEndpoint::send( int port, ThinIPCMessage * msg ) {
    CRASH_REPORT_BEGIN;
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
        
//        std::cout << "INFO: Sending chunk("<< clen << ") to " << port << std::endl;
        
        // Send frame
        socklen_t flen = sizeof(server);
        len = sendto( this->sock, buf, clen, 0, (struct sockaddr *)&server, flen );
        
        // Check errors
        if (len <= 0) return len;
        ctotal += len;
        
    }
    
    // Return bytes sent
    return ctotal;
    CRASH_REPORT_END;
}

/**
 * Wait for a data frame on the socket
 */
int ThinIPCEndpoint::recv( int * port, ThinIPCMessage * msg ) {
    CRASH_REPORT_BEGIN;
    char buf[1024];
    int len;
    
    // Prepare the address to listen on
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = 0;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Start receiving frame
    msg->reset();
    while (true) {
        
        // Fetch frame
        socklen_t flen = sizeof(server);
        len = recvfrom( this->sock, buf, 1024, 0, (struct sockaddr *)&server, &flen );
        if (len <= 0) return len;
        
        // Update the remote port
        *port = ntohs( server.sin_port );
        
        // Push on IPC frame
        len = msg->putChunk( buf, 1024 );
        if (len > 0) return len;
        
    }
    
    CRASH_REPORT_END;
}

/**
 * Check if there are messages pending in the IPC chain
 */
bool ThinIPCEndpoint::isPending( int timeout ) {
    CRASH_REPORT_BEGIN;
    
    /* Prepare timeout */
    struct timeval tv;
    tv.tv_usec = timeout;
    tv.tv_sec = 0;
    
    /* Prepare descriptors */
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(this->sock, &rfds);

    /* Wait for read on socket for timeout */
    int retval = select(this->sock+1, &rfds, NULL, NULL, &tv);

    /* Check for result */
    if (retval == -1)
        return false;   // Error
     else if (retval)
         return true;   // Has data
     else
         return false;  // Timed out

    CRASH_REPORT_END;
}
