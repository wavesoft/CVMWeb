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
#include <vector>
#include <map>
#include <time.h>

#include <boost/thread.hpp>

#include "Hypervisor.h"
#include "Virtualbox.h"
#include "ThinIPC.h"
#include "LocalConfig.h"
#include "platform.h"

#include "../DaemonCtl.h"

using namespace std;

// Delay between time-consuming operations
#define             SLOW_TIMER      30

// Delay between time-critical operations
#define             FAST_TIMER       5

ThinIPCEndpoint     * ipc;
Hypervisor          * hv;
time_t                reloadTimer;
time_t                probeTimer;
int                   idleTime;
bool                  isIdle = false;
bool                  isAlive = true;
LocalConfigPtr        config;

boost::mutex          sessionsMutex;
boost::thread         reloadThread;
bool                  reloadTriggered = false;

vector<int>           monitoredPids;

/**
 * Switch the idle states of the VMs
 */
void switchIdleStates( bool idle ) {
    if (!isAlive) return;
    map<string,string> emptyMap;

    /* Acquire mutex between switchIdleStates & reloadSessions */
    sessionsMutex.lock();

    /* Pause all the VMs if we are not idle */
    if (!idle) {
        
        for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
            HVSession* sess = *i;
            if (sess->daemonControlled) {
                cout << "INFO: Switching to idle VM " << sess->uuid << " (" << sess->name << ")" << endl;
                
                /* If daemonMinCap == 0 then either pause or suspend the vm */
                if (sess->daemonMinCap == 0) {
                    
                    if (sess->state == STATE_STARTED) {
                        if ((sess->daemonFlags & DF_SUSPEND) != 0) {
                            cout << "INFO: Suspending VM " << sess->uuid << " (" << sess->name << ")" << endl;
                            sess->hibernate();
                        } else {
                            cout << "INFO: Pausing VM " << sess->uuid << " (" << sess->name << ")" << endl;
                            sess->pause();
                        }
                    }
                    
                /* Otherwise, set the execution cap to the given value */
                } else {
                    
                    /* Having a cap value means that we want the VM running. If we also 
                      have the DF_AUTOSTART flag, start the VM now. */
                    if ((sess->state == STATE_OPEN) && ((sess->daemonFlags & DF_AUTOSTART) != 0)) {
                        cout << "INFO: Starting VM " << sess->uuid << " (" << sess->name << ")" << endl;
                        sess->start( NULL ); // Blank user-data means 'keep the context iso untouched'
                    }
                    
                    cout << "INFO: Setting cap to " << sess->daemonMinCap << " for VM " << sess->uuid << " (" << sess->name << ")" << endl;
                    sess->setExecutionCap( sess->daemonMinCap );
                }
            }
        }
        
    } else {
        
        for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
            HVSession* sess = *i;
            if (sess->daemonControlled) {
                cout << "INFO: Switching to active VM " << sess->uuid << " (" << sess->name << ")" << endl;

                /* If daemonMinCap == 0 then either start or resume the vm */
                if (sess->daemonMinCap == 0) {
                    
                    /* If we have the DF_SUSPEND flag, start the vm */
                    if ( (sess->daemonFlags & DF_SUSPEND) != 0 ) {
                        cout << "INFO: Starting VM " << sess->uuid << " (" << sess->name << ")" << endl;
                        sess->start( NULL ); // Blank user-data means 'keep the context iso untouched'
                        
                    /* Otherwise, resume the VM if it's paused */
                    } else if (sess->state == STATE_PAUSED) {
                        cout << "INFO: Resuming VM " << sess->uuid << " (" << sess->name << ")" << endl;
                        sess->resume();
                        
                    /* If we have the DF_AUTOSTART flag, start the vm */
                    } else if ((sess->state == STATE_OPEN) || ((sess->daemonFlags & DF_AUTOSTART) != 0 )) {
                        cout << "INFO: Starting VM " << sess->uuid << " (" << sess->name << ")" << endl;
                        sess->start( NULL ); // Blank user-data means 'keep the context iso untouched'
                        
                    }
                    
                /* Otherwise, set the execution cap to the given value */
                } else {
                    
                    /* Having a cap value means that we want the VM running. If we also 
                      have the DF_AUTOSTART flag, start the VM now. */
                    if ((sess->state == STATE_OPEN) && ((sess->daemonFlags & DF_AUTOSTART) != 0)) {
                        cout << "INFO: Starting VM " << sess->uuid << " (" << sess->name << ")" << endl;
                        sess->start( NULL ); // Blank user-data means 'keep the context iso untouched'
                    }
                    
                    /* Set execution cap */
                    cout << "INFO: Setting cap to " << sess->daemonMaxCap << " for VM " << sess->uuid << " (" << sess->name << ")" << endl;
                    sess->setExecutionCap( sess->daemonMaxCap );
                    
                }
            }
        }
        
    }
    
    /* Release mutex between switchIdleStates & reloadSessions */
    sessionsMutex.unlock();
}

/**
 * Reload thread
 */
void reloadSessions_thread() {
    if (!isAlive) return;

    /* Acquire mutex between switchIdleStates & reloadSessions */
    sessionsMutex.lock();

    cout << "INFO: Reloading sessions" << endl;
    hv->loadSessions();
    
    // Check if we don't need the daemon any more
    bool needsDaemon = false;
    for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->daemonControlled) {
            needsDaemon = true;
            break;
        }
    }

    /* Release mutex between switchIdleStates & reloadSessions */
    sessionsMutex.unlock();

    // If we don't need the daemon, Initiate graceful shutdown
    if (!needsDaemon) {
        cout << "INFO: I am not needed. Bye bye!" << endl;
        isAlive = false;
    }

    // Release reload lock
    reloadTriggered = false;
}

/**
 * Trigger a session reload on another thread
 */
void reloadSessions() {
    if (!isAlive) return;
    if (reloadTriggered) return;
    reloadTriggered = true;
    reloadThread = boost::thread( &reloadSessions_thread );
}

/** 
 * Reap dead sessions if they have the DF_AUTODESTROY flag
 */
void reapDead() {
    if (!isAlive) return;
    bool needsUpdate = false;
    cout << "[INFO] reapDead";
    sessionsMutex.lock();
    cout << " started" << endl;
    for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
        HVSession* sess = *i;
        if (sess->daemonControlled && ((sess->daemonFlags & DF_AUTODESTROY) != 0) ) {
            
            // Update sesion status
            sess->updateFast();

            // If it's closed, destroy
            if (sess->state == STATE_OPEN) {
                needsUpdate = true;
                sess->close();
            }

        }
    }
    sessionsMutex.unlock();

    // If we need update, schedule sessions reload
    if (needsUpdate) {
        reloadSessions();
    }

}

/**
 * Asynchronous UDP server thread
 */
void serverThread() {
    std::cout << "[UDP] Started listening on port " << DAEMON_PORT << std::endl;
    ipc = new ThinIPCEndpoint( DAEMON_PORT );
    while (isAlive) {
        
        /* Check if we have IPC data to read 
           (This also acts as our CPU throttling delay) */
        if (ipc->isPending( 250000 )) {
            
            /* Read IPC data */
            int port;
            static ThinIPCMessage msg, ans;
            ipc->recv( &port, &msg );
            
            /* Handle data */
            ans.reset();
            int iAction = msg.readShort();
            cout << "INFO: Msg=" << iAction << ", From=" << port << endl;
            
            if (iAction == DIPC_IDLESTATE) { // Get the idle state
                ans.writeShort(DIPC_ANS_OK);
                ans.writeShort( isIdle ? 1 : 0 );
                
            } else if (iAction == DIPC_SHUTDOWN) { // Shut down the daemon
                ans.writeShort(DIPC_ANS_OK);
                ans.writeShort( 0 );
                ipc->send( port, &ans );
                break;
                
            } else if (iAction == DIPC_SET_IDLETIME) { // Set the idle time value
                idleTime = msg.readShort();
                ans.writeShort(DIPC_ANS_OK);
                ans.writeShort(idleTime);
                config->setNum<int>( "idle-time", idleTime );
                
            } else if (iAction == DIPC_GET_IDLETIME) { // Return the current idle time settings
                ans.writeShort(DIPC_ANS_OK);
                ans.writeShort(idleTime);
                
            } else if (iAction == DIPC_RELOAD) { // Manually reload sessions
                ans.writeShort(DIPC_ANS_OK);
                reloadTimer = time(NULL);
                reloadSessions();
                
            } else {
                ans.writeShort(DIPC_ANS_ERROR);
            }
            
            /* Send responpse */
            ipc->send( port, &ans );
        }
        
    }
    isAlive = false;
}

/**
 * Check if system is idle
 */
bool isSystemIdle() {
    /* Acquire mutex between switchIdleStates & reloadSessions */
    sessionsMutex.lock();

    /* Use resource detection when available */
    bool enableResourceDetection = true;

    /* Check what PIDs were found in the sessions */
    vector<int> usedPids;

    /* Calculate the CPU usage of each PID */
    platformBeginMeasurement();
    for (vector<HVSession*>::iterator i = hv->sessions.begin(); i != hv->sessions.end(); i++) {
        HVSession * sess = *i;
        if (sess->daemonControlled) {

            /* If we don't have a PID it means that at least one hypervisor
             * process won't be calculated on the resources calculation mechanism
             * which also means that we cannot trust the CPU usage of the system
             * (again: because there might be a hypervisor running that WE instantiated,
             *  but the system does not grant as access).
             *
             * This means we should disable the more advanced Resource-based idle
             * detection, and keep only the last input time. */
            if (sess->pid == 0) {
                enableResourceDetection = false;
                break;
            }

            /* If this PID is not monitored, start monitoring */
            if (std::find( monitoredPids.begin(), monitoredPids.end(), sess->pid) == monitoredPids.end() ) {
                monitoredPids.push_back( sess->pid );
                platformStartMonitorPID( sess->pid );
            }

            /* Mark this PID as used */
            usedPids.push_back( sess->pid );

            /* Get CPU Usage */
            cout << "[INFO] Checking session " << sess->uuid << " (Pid " << sess->pid << ")" << endl;
            platformCPUProcessUsage( sess->pid );
        }
    }
    platformEndMeasurement();

    /* Check which PIDs were not used any more and stop monitoring them */
    vector<int>::iterator it = monitoredPids.begin();
    for ( ; it != monitoredPids.end(); ) {
        int pid = *it;
        if (std::find( usedPids.begin(), usedPids.end(), pid) == usedPids.end() ) {

            /* PID Not used any more */
            platformStopMonitorPID( pid );

            /* Delete PID from monitoredPids */
            monitoredPids.erase( it );

        } else {
            ++it;
        }
    }

    /* Release mutex */
    sessionsMutex.unlock();
    return false;
}

/**
 * Entry point
 */
int main( int argc, char ** argv ) {
    bool newIdleState;
    
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
    std::string lockFile = getDaemonLockfile();
    if (isDaemonRunning()) {
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
    thinIPCInitialize();

    /* Initialize platform metrics */
    platformInit();

    /* Initialize arguments */
    config = LocalConfig::global();
    idleTime = config->getNumDef<int>( "idle-time", 30 );
    config->setNum("idle-time", idleTime);
    cout << "[INFO] Using idle-time: " << idleTime << endl;
    
    /* Reset state */
    reloadTimer = time( NULL );
    probeTimer = time( NULL );
    
    /* Start server thread */
    boost::thread tServer(&serverThread);
    
    /* Main loop */
    std::cout << "[Main] Processing events" << std::endl;
    while (isAlive) {
        time_t cTime = time( NULL );
        
        /* Perform slow operations */
        if ( cTime > ( reloadTimer + SLOW_TIMER ) ) {

            // Do extensive reload only on idle
            if (isIdle) {
                cout << "INFO: Reloading sessions" << endl;
                reloadSessions();
                reloadTimer = cTime;
            }
            
        }

        /* Perform probe operations */
        if ( cTime > ( probeTimer + FAST_TIMER ) ) {
            probeTimer = cTime;

            // Reap dead sessions
            reapDead();

        }
        
        /* Check for idle state switch */
        //isSystemIdle();
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
                reloadSessions();
                if (!isAlive) break;

                cout << "INFO: Switching to IDLE state" << endl;
                switchIdleStates( true );
            }
        }
        
        /* Do not create CPU load on the loop */
        sleepMs(1000);
        
    }
    
    /* Cleanup */
    daemonUnlock( lockInfo );
    delete(config);
    platformCleanup();

    /* Graceful cleanup */
    return 0;
    
}
