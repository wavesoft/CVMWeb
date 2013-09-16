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

#include "../platform.h"
#include "../../Hypervisor.h"
#include "../../Utilities.h"
#include <Windows.h>
#include <WinBase.h>
#include <WinUser.h>

#include <iostream>
#include <vector>
#include <map>
using namespace std;

/**
 * Struct to keep the previous time information
 * in order to calculate diff between consecutive
 * platformCPUProcessUsage calls.
 */
typedef struct {
	ULONGLONG kernelTime, userTime, idleTime;
	unsigned long mTime;
} COUNTER_TIMES;

/**
 * Map of process times and system time
 */
map<int, COUNTER_TIMES*> 	procTimes;
COUNTER_TIMES				systemTime, systemDelta;

/**
 * Initialize platform-dependent code stack
 */
int platformInit ( ) {

    // Reset timers
    memset(&systemTime, 0, sizeof(COUNTER_TIMES));
    memset(&systemDelta, 0, sizeof(COUNTER_TIMES));

    // Return OK
    return 0;

}

/**
 * Cleanup platform-dependent code stack
 */
int platformCleanup ( ) {

    // Nothing to do, really
    return 0;

}

/**
 * Get the number of seconds since the last time the user
 * sent an input to the system.
 */
int platformIdleTime( ) {
    LASTINPUTINFO plii = {0};
	DWORD msTime;
	plii.cbSize = sizeof( LASTINPUTINFO );
	plii.dwTime = 0;

    GetLastInputInfo( &plii );
	msTime = GetTickCount();
	msTime -= plii.dwTime;
    msTime /= 1000;
    
    return (int)msTime;
}

/**
 * Begin resource sampling routines
 */
int platformBeginMeasurement( ) {

    // Get system CPU times
	FILETIME ftSysIdle, ftSysKernel, ftSysUser;
    GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser);

    // Convert to large integers
    LARGE_INTEGER qSysKernel;   qSysKernel.HighPart = ftSysKernel.dwHighDateTime;   qSysKernel.LowPart = ftSysKernel.dwLowDateTime;
    LARGE_INTEGER qSysUser;     qSysUser.HighPart = ftSysUser.dwHighDateTime;       qSysUser.LowPart = ftSysUser.dwLowDateTime;
    LARGE_INTEGER qSysIdle;     qSysIdle.HighPart = ftSysIdle.dwHighDateTime;       qSysIdle.LowPart = ftSysIdle.dwLowDateTime;

    // If we had a previous sampling, do the delta
    if (systemTime.mTime != 0) {

        // Delta times
        systemDelta.kernelTime = qSysKernel.QuadPart - systemTime.kernelTime;
        systemDelta.userTime = qSysUser.QuadPart - systemTime.userTime;
        systemDelta.idleTime = qSysIdle.QuadPart - systemTime.idleTime;

        // Set the time of the last sampling
        systemDelta.mTime = getMillis() - systemTime.mTime;

    }

    // Update last sample
    systemTime.kernelTime = qSysKernel.QuadPart;
    systemTime.userTime = qSysUser.QuadPart;
    systemTime.idleTime = qSysIdle.QuadPart;

    // Get the time of the last sampling
    systemTime.mTime = getMillis();

    // **DEBUG**
    cout << "-- System Times --" << endl;
    cout << "   KERNEL: " << qSysKernel.QuadPart << endl;
    cout << "     IDLE: " << qSysIdle.QuadPart << endl;
    cout << "     USER: " << systemTime.userTime << endl;
    cout << "D(KERNEL): " << systemDelta.kernelTime << endl;
    cout << "  D(USER): " << systemDelta.userTime << endl;
    cout << "  D(IDLE): " << systemDelta.idleTime << endl;
    cout << "    D(ms): " << systemDelta.mTime << endl;

    // Return OK
    return 0;

}

/**
 * End resource sampling routines
 */
int  platformEndMeasurement  ( ) {

    // Nothing particular here
    return 0;

}


/**
 * Start monitoring a particular PID
 */
int platformStartMonitorPID ( int pid ) {

	// If we already have this pid, exit
    if (procTimes.find(pid) != procTimes.end())
        return HVE_ALREADY_EXISTS;

	// Allocate counter space
	COUNTER_TIMES * pTime = new COUNTER_TIMES();
    memset( pTime, 0, sizeof(COUNTER_TIMES) );

    // Store to pid vector
    procTimes[pid] = pTime;

    // OK
    return HVE_OK;

}

/**
 * Stop monitoring a particular PID
 */
int platformStopMonitorPID  ( int pid ) {

	// If we don't have this pid, exit
    if (procTimes.find(pid) == procTimes.end())
        return HVE_NOT_FOUND;

    // Dispose counter
    COUNTER_TIMES * pTime = procTimes[pid];
    delete pTime;

    // Remove from vector
    procTimes.erase( pid );

    // OK
    return HVE_OK;

}

/**
 * Shorthand to diff FILETIMEs
 */
ULONGLONG __diffTimes(const FILETIME& ftA, const FILETIME& ftB) {
    LARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    return a.QuadPart - b.QuadPart;
}


/**
 * Get the CPU Usage of the given application, divided to the total
 * CPU usage of the machine. The value should be normalized to 100 (percent)
 */
int platformCPUProcessUsage ( int pid ) {
    int percent = 0;

	// If we don't monitor this pid, exit
    if (procTimes.find(pid) == procTimes.end())
        return HVE_NOT_FOUND;

    // If we don't have system info, it means the 
    // user did not call platformBeginMeasurement()
    if (systemTime.mTime == 0)
        return HVE_USAGE_ERROR;

    // Open process handle
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION , FALSE, pid );
	if (hProcess == NULL) return HVE_NOT_ALLOWED;

    // Fetch process times structure
    COUNTER_TIMES * pTime = procTimes[pid];

    // Prepare variables
  	FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;

  	// Extract process times
    if ( !GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser))
    	return HVE_EXTERNAL_ERROR;

    // Cast values to large integers
    LARGE_INTEGER qProcKernel;  qProcKernel.HighPart = ftProcKernel.dwHighDateTime; qProcKernel.LowPart = ftProcKernel.dwLowDateTime; 
    LARGE_INTEGER qProcUser;  qProcUser.HighPart = ftProcUser.dwHighDateTime; qProcUser.LowPart = ftProcUser.dwLowDateTime; 

    // **DEBUG**
    cout << "-- Process [" << pid << "] Times --" << endl;

    // Do diff if we have data, otherwise return 0
    if (pTime->mTime == 0) {

        cout << "(No data yet)" << endl;

    } else {

        // Calculate Deltas
        ULONGLONG procDeltaKernel = qProcKernel.QuadPart - pTime->kernelTime;
        ULONGLONG procDeltaUser = qProcUser.QuadPart - pTime->userTime;

        // Sum deltas
        ULONGLONG nProcTime = procDeltaKernel + procDeltaUser;
        ULONGLONG nProcSystem = systemDelta.userTime + systemDelta.kernelTime; 

        unsigned long msDelta = getMillis() - pTime->mTime;

        // **DEBUG**
        cout << "D(KERNEL): " << qProcKernel.QuadPart << endl;
        cout << "  D(USER): " << qProcUser.QuadPart << endl;
        cout << "    D(ms): " << msDelta << endl;

        // Calculate delta
        if (nProcSystem > 0) {
            short m_nCpuUsage = (short)((100.0 * nProcTime) / nProcSystem);
            percent = (int)m_nCpuUsage;

            // **DEBUG**
            cout << "    CPU %: " << m_nCpuUsage << endl;
        }


    }

    // Store values to the process time structure
    pTime->kernelTime = qProcKernel.QuadPart;
    pTime->userTime = qProcUser.QuadPart;
    pTime->mTime = getMillis();

    // Return percent
    return percent;


}

/**
 * Get the overall system CPU usage, normalized to 100 (percent)
 */
int platformCPUGlobalUsage ( ) {
    return 0;
}
