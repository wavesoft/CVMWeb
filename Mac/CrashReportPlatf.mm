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

#include <boost/thread.hpp>
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <string>
 
#include "../CrashReport.h"

/**
 * Get platform string
 */
std::string crashReportPlatformString() {

	// Open SystemVersion plist
	NSDictionary *systemVersionDictionary =
	    [NSDictionary dictionaryWithContentsOfFile:
	        @"/System/Library/CoreServices/SystemVersion.plist"];

	// Extract product version
	NSString *systemVersion =
	    [systemVersionDictionary objectForKey:@"ProductVersion"];

	// Return string
	std::string *stdStrVersion = new std::string([systemVersion UTF8String]);
	return *stdStrVersion;
	
}