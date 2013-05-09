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

#include <cstdlib>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <time.h>

#include "Hypervisor.h"
#include "Virtualbox.h"
#include "ThinIPC.h"
#include "platform.h"

#include "../DaemonCtl.h"

using namespace std;

#define             RELOAD_TIME     30

ThinIPCEndpoint     * ipc;
Hypervisor          * hv;
time_t                reloadTimer;
int                   idleTime;
bool                  isIdle = false;

/**
 * Switch the idle states of the VMs
 */
void switchIdleStates( bool idle ) {
    
    /* Pause all the VMs if we are not idle */
    if (!idle) {
        
        for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
            HVSession* sess = *i;
            if (sess->state == STATE_STARTED ) {
                cout << "INFO: Pausing VM " << sess->uuid << " (" << sess->name << ")" << endl;
                sess->pause();
            }
        }
        
    } else {
        
        for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
            HVSession* sess = *i;
            if (sess->state == STATE_PAUSED ) {
                cout << "INFO: Resuming VM " << sess->uuid << " (" << sess->name << ")" << endl;
                sess->resume();
            }
        }
        
    }
    
}

/**
 * Entry point
 */
int main( int argc, char ** argv ) {
    
    /* Get a hypervisor control instance */
    hv = detectHypervisor();
    if (hv == NULL) {
        cerr << "ERROR: Unable to detect hypervisor!\n";
        return 3;
    }
    
    /* This is working only with VirtualBox */
    if (hv->type != HV_VIRTUALBOX) {
        cerr << "ERROR: Only VirtualBox is currently supproted!\n";
        return 4;
    }
    
    /* Lock file */
    DLOCKINFO * lockInfo;
    std::string lockFile = getDaemonLockfile( argv[0] );
    if (isDaemonRunning(lockFile)) {
        /* Already locked */
        cerr << "ERROR: Another daemon process is running!\n";
        return 2;
    }
    if ( (lockInfo = daemonLock(lockFile)) == NULL ) {
        /* Could not lock */
        cerr << "ERROR: Could not acquire lockfile\n";
        return 2;
    }
    
    /* Initialize ThinIPC */
    ThinIPCInitialize();
    
    /* Initialize arguments */
    ipc = new ThinIPCEndpoint( DAEMON_PORT );
    idleTime = 30;
    
    /* Reset state */
    reloadTimer = time( NULL );
    
    /* Main loop */
    for (;;) {
        
        /* Check if we have IPC data to read 
           (This also acts as our CPU throttling delay) */
        if (ipc->isPending( 250000 )) {
            
            /* Read IPC data */
            int port;
            static ThinIPCMessage msg, ans;
            ipc->recv( &port, &msg );
            
            /* Handle data */
            ans.reset();
            int iAction = msg.readInt();
            cout << "INFO: Msg=" << iAction << ", From=" << port << endl;
            
            if (iAction == DIPC_IDLESTATE) { // Get the idle state
                ans.writeInt(DIPC_ANS_OK);
                ans.writeInt( isIdle ? 1 : 0 );
                
            } else if (iAction == DIPC_SHUTDOWN) { // Shut down the daemon
                ans.writeInt(DIPC_ANS_OK);
                ans.writeInt( 0 );
                ipc->send( port, &ans );
                break;
                
            } else if (iAction == DIPC_SET_IDLETIME) { // Set the idle time value
                idleTime = msg.readInt();
                ans.writeInt(DIPC_ANS_OK);
                ans.writeInt(idleTime);
                
            } else if (iAction == DIPC_GET_IDLETIME) { // Return the current idle time settings
                ans.writeInt(DIPC_ANS_OK);
                ans.writeInt(idleTime);
                
            } else {
                ans.writeInt(DIPC_ANS_ERROR);
            }
            
            /* Send responpse */
            ipc->send( port, &ans );
            
        }
        
        /* Check for state reload times */
        if ( time( NULL ) > ( reloadTimer + RELOAD_TIME ) ) {
            if (isIdle) { // Reload only on IDLE state
                cout << "INFO: Reloading sessions" << endl;
                hv->loadSessions();
                reloadTimer = time(NULL);
            }
        }
        
        /* Check for idle state switch */
        if (isIdle) {
            if ( platformIdleTime() < idleTime ) {
                isIdle = false;
                cout << "INFO: Switching to ACTIVE state" << endl;
                switchIdleStates( false );
            }
        } else {
            if ( platformIdleTime() >= idleTime ) {
                isIdle = true;
                cout << "INFO: Reloading sessions" << endl;
                hv->loadSessions();
                cout << "INFO: Switching to IDLE state" << endl;
                switchIdleStates( true );
            }
        }
        
    }
    
    /* Unlock lockfile */
    daemonUnlock( lockInfo );
    
    /* Graceful cleanup */
    return 0;
    
}
