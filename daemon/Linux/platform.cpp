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

int platformMeasureResources( PLAF_USAGE * usage ) {
     return HVE_NOT_IMPLEMENTED;
}

/**
 * Return the system idle time (in seconds)
 */
int platformIdleTime( ) {
    time_t idle_time;
    static XScreenSaverInfo *mit_info;
    Display *display;
    int screen;
    
    /* Allocate screensaver info */
    mit_info = XScreenSaverAllocInfo();
    if (mit_info == NULL)
        return HVE_HVE_EXTERNAL_ERROR;
    
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