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
 
#include "../../Hypervisor.h"
#include "../platform.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

int platformMeasureResources( PLAF_USAGE * usage ) {
    return HVE_NOT_IMPLEMENTED;
}

int platformIdleTime() {
    mach_port_t port;
    io_iterator_t iter;
    CFTypeRef value = kCFNull;
    uint64_t idle = 0;
    CFMutableDictionaryRef properties = NULL;
    io_registry_entry_t entry;

    IOMasterPort(MACH_PORT_NULL, &port);
    IOServiceGetMatchingServices(port, IOServiceMatching("IOHIDSystem"), &iter);
    if (iter) {
        if ((entry = IOIteratorNext(iter))) {
            if (IORegistryEntryCreateCFProperties(entry, &properties, kCFAllocatorDefault, 0) == KERN_SUCCESS && properties) {
                if (CFDictionaryGetValueIfPresent(properties, CFSTR("HIDIdleTime"), &value)) {
                    if (CFGetTypeID(value) == CFDataGetTypeID()) {
                        CFDataGetBytes( (CFDataRef) value, CFRangeMake(0, sizeof(idle)), (UInt8 *) &idle);
                    } else if (CFGetTypeID(value) == CFNumberGetTypeID()) {
                        CFNumberGetValue( (CFNumberRef)value, kCFNumberSInt64Type, &idle);
                    }
                }
                CFRelease(properties);
            }
            IOObjectRelease(entry);
        }
        IOObjectRelease(iter);
    }

    double fTime = idle / 1000000000.0;
    return (int)fTime;
}
