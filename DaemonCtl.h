
#ifndef DAEMONCTL_H_4LFX6MZT
#define DAEMONCTL_H_4LFX6MZT

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

/* Config constants */
#define DAEMON_PORT         58740

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
