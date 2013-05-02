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

#ifndef THINIPC_H_MPQIX21W
#define THINIPC_H_MPQIX21W

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#define HANDLE          int
#define MAX_MSG_SIZE    1024

#define SCKE_OK         0
#define SCKE_CREATE     -1
#define SCKE_BIND       -2
#define SCKE_LISTEN     -3
#define SCKE_SEND       -4
#define SCKE_RECV       -5

/**
 * The description of the payload that will be exchanged
 */
class ThinIPCMessage {
public:
    
    ThinIPCMessage( );
    ThinIPCMessage( char* payload, short size );
    virtual ~ThinIPCMessage( );
    
    /* Shorthands */
    static ThinIPCMessage       __shorthandInstance;
    static ThinIPCMessage *     I( int );
    static ThinIPCMessage *     S( std::string );
    static ThinIPCMessage *     II( int, int );
    static ThinIPCMessage *     SS( std::string, std::string );
    static ThinIPCMessage *     IS( int, std::string );
    static ThinIPCMessage *     SI( std::string, int );
    
    /* Object I/O */
    int                         readInt();
    std::string                 readString();
    template <typename T> short readPtr( T * ptr );
    short                       writeInt( int v );
    short                       writeString( std::string v );
    template <typename T> short writePtr( T * ptr );
    
    /* Internals */
    void                        rewind();
    void                        reset();
    short                        __ioPos;
    char *                      data;
    short                       size;
    
    /* I/O Chunking */
    short                       getChunk( char * buffer, short capacity );
    short                       putChunk( char * buffer, short capacity );
    short                       __chunkPos;
};

/**
* IPC Endpoint
 */
class ThinIPCEndpoint {
public:
    
    ThinIPCEndpoint( int port );
    virtual ~ThinIPCEndpoint();
    
    int                         recv        ( int port, ThinIPCMessage * msg );
    int                         send        ( int port, ThinIPCMessage * msg );
    
protected:
    HANDLE                      sock;
    int                         errorCode;
    
};

int                             ThinIPCInitialize();

#endif /* end of include guard: THINIPC_H_MPQIX21W */
