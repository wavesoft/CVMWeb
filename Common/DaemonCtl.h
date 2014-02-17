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

#ifndef DAEMONCTL_H_4LFX6MZT
#define DAEMONCTL_H_4LFX6MZT

#include "Config.h"
#include "ThinIPC.h"
#include "CrashReport.h"
#include <string>

/* Used by daemonLock/~Unlock */
typedef struct {
    std::string     fname;
} DLOCKINFO;

/* Daemon IPC Actions */
#define DIPC_IDLESTATE      0x0001
#define DIPC_SHUTDOWN       0x0002
#define DIPC_SET_IDLETIME   0x0003
#define DIPC_GET_IDLETIME   0x0004
#define DIPC_RELOAD         0x0008

/* Daemon IPC Responses */
#define DIPC_ANS_OK         0x0001
#define DIPC_ANS_ERROR      0xFFFF

/* Daemon flags */
#define DF_SUSPEND          0x01    // Suspend VM instead of pausing
#define DF_AUTOSTART        0x02    // If the VM is down, start it with blank userData
#define DF_AUTODESTROY      0x04    // If the VM is down, destroy it

/* Daemon controlling functions */
std::string             getDaemonLockfile   ( );
bool                    isDaemonRunning     ( );
short int               daemonIPC           ( ThinIPCMessage * send, ThinIPCMessage * recv );
short int               daemonGet           ( short int action );
short int               daemonSet           ( short int action, short int value );
int                     daemonStart         ( std::string path_to_bin );
int                     daemonStop          ( );

/* Used by the daemon process */
DLOCKINFO *             daemonLock          ( std::string lockfile );
void                    daemonUnlock        ( DLOCKINFO * file );

#endif /* end of include guard: DAEMONCTL_H_4LFX6MZT */
