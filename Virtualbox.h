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

#ifndef VIRTUALBOX_H
#define VIRTUALBOX_H

#include "Hypervisor.h"
#include <map>

/* Forward decleration */
class Virtualbox;

/**
 * VirtualBox Session
 */
class VBoxSession : public HVSession {
public:
    
    std::string             uuid;
    Virtualbox *            host;
    
    virtual int             pause();
    virtual int             close();
    virtual int             resume();
    virtual int             reset();
    virtual int             stop();
    virtual int             open( int cpus, int memory, int disk, std::string cvmVersion );
    virtual int             start( std::string userData );
    virtual int             setExecutionCap(int cap);
    virtual int             setProperty( std::string name, std::string key );
    virtual std::string     getProperty( std::string name );
    
};

/**
 * VirtualBox Hypervisor
 */
class Virtualbox : public Hypervisor {
public:

    std::string             hvGuestAdditions;
    
    /* Internal parameters */
    std::string             getHostOnlyAdapter  ();
    std::string             getProperty         ( std::string vm, std::string name );
    std::map<std::string, 
        std::string>        getMachineInfo      ( std::string uuid );
    int                     getMachineUUID      ( std::string name, std::string * uuid );
    int                     setProperty         ( std::string vm, std::string name, std::string value );
    int                     startVM             ( std::string uuid );
    int                     controlVM           ( std::string uuid, std::string how );
    int                     prepareSession      ( VBoxSession * session );

    /* Overloads */
    virtual int             loadSessions        ( );
    virtual HVSession *     allocateSession     ( std::string name, std::string key );
    
};

#endif /* end of include guard: VIRTUALBOX_H */
