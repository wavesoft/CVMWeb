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

// VS: MMVJ9-FKY74-W449Y-RB79G-8GJGJ 

#ifndef PLATFORM_H_X4S3J8HJ
#define PLATFORM_H_X4S3J8HJ

/**
 * You should call these functions in the following way:
 *
 * - platformInit()                        : To your startup script
 *  - platformStartMonitorPID( pid )       : To start resource monitoring of the give PID
 *  - platformBeginMeasurement( )          : Before calling the resource-measuring functions
 *   - platformCPUProcessUsage( pid )      : To get CPU usage of the give PID
 *   - platformCPUGlobalUsage( )           : To get the global CPU usage
 *  - platformEndMeasurement( )            : To complete resource measurements
 *
 *  - platformStopMonitorPID( pid )        : When you don't care any more for the given PID
 * - platformCleanup()                    : Before your application exists
 */

/**
 * Initialize platform-dependent code stack
 */
int 					platformInit			( );

/**
 * Cleanup platform-dependent code stack
 */
int 					platformCleanup			( );

/**
 * Get the number of seconds since the last time the user
 * sent an input to the system.
 */
int                     platformIdleTime		( );

/**
 * Start monitoring a particular PID
 */
int						platformStartMonitorPID	( int pid );

/**
 * Stop monitoring a particular PID
 */
int						platformStopMonitorPID	( int pid );

/**
 * Begin resource sampling routines
 */
int                     platformBeginMeasurement( );

/**
 * End resource sampling routines
 */
int                     platformEndMeasurement  ( );

/**
 * Get the CPU Usage of the given application, divided to the total
 * CPU usage of the machine. The value should be normalized to 100 (percent)
 */
int						platformCPUProcessUsage	( int pid );

/**
 * Get the overall system CPU usage, normalized to 100 (percent)
 */
int						platformCPUGlobalUsage	( );

#endif /* end of include guard: PLATFORM_H_X4S3J8HJ */
