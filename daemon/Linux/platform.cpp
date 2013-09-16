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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>

/**
 * Initialize platform-dependent code stack
 */
int platformInit ( ) {
    return 0;
}

/**
 * Cleanup platform-dependent code stack
 */
int platformCleanup ( ) {
    return 0;
}

/**
 * Get the number of seconds since the last time the user
 * sent an input to the system.
 */
int platformIdleTime( ) {
    time_t idle_time;
    static XScreenSaverInfo *mit_info;
    Display *display;
    int screen;
    
    /* Allocate screensaver info */
    mit_info = XScreenSaverAllocInfo();
    if (mit_info == NULL)
        return HVE_EXTERNAL_ERROR;
    
    /* Open display */
    if ((display=XOpenDisplay(NULL)) == NULL) 
        return HVE_EXTERNAL_ERROR;
    
    /* Fetch default screen */
    screen = DefaultScreen(display);
    
    /* Query info */
    if (XScreenSaverQueryInfo(display, RootWindow(display,screen), mit_info) == 0)
        return HVE_NOT_SUPPORTED;
    
    /* Calculate idle time in seconds */
    idle_time = (mit_info->idle) / 1000;
    
    /* Cleanup */
    XFree(mit_info);
    XCloseDisplay(display); 
    
    return idle_time;
}

/**
 * Start monitoring a particular PID
 */
int platformStartMonitorPID ( int pid ) {
    return 0;
}

/**
 * Stop monitoring a particular PID
 */
int platformStopMonitorPID  ( int pid ) {
    return 0;
}

/**
 * Begin resource sampling routines
 */
int platformBeginMeasurement( ) {
    return 0;
}

/**
 * End resource sampling routines
 */
int  platformEndMeasurement  ( ) {
    return 0;
}

/**
 * Get the CPU Usage of the given application, divided to the total
 * CPU usage of the machine. The value should be normalized to 100 (percent)
 */
int platformCPUProcessUsage ( int pid ) {
    return 0;
}

/**
 * Get the overall system CPU usage, normalized to 100 (percent)
 */
int platformCPUGlobalUsage ( ) {
    return 0;
}
