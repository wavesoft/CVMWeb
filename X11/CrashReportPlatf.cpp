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

#include "../CrashReport.h"
using namespace std;

#include <stdio.h>
#include <stdlib.h>

/**
 * Drain the file descriptior and return the contents
 */
string drainFP( FILE * fp ) {
	char outDat[1035];
	string ans = "";

	/* Read the output a line at a time */
	while (fgets(outDat, sizeof(outDat)-1, fp) != NULL) {
		ans += outDat;
	}

	/* Trim \n from the end */
	ans = ans.substr(0, ans.length()-1);

	/* Return FILE* contents */
	return ans;
}

/**
 * Using popen to get stdout
 * (We cannot use sysExec, because it might have caused the exception)
 */
string execAndGet( const char * cmd ) {

	FILE *fp;
	string ans;

	/* Open the command for reading. */
	fp = popen(cmd, "r");
	if (fp == NULL) { return ""; };

	/* Drain FP */
	ans = drainFP( fp );

	/* close and return data */
	pclose(fp);
	return ans;

}

/**
 * Open and read file
 */
string openAndGet( const char * file ) {

	FILE *fp;
	string ans;

	/* Open the command for reading. */
	fp = fopen(file, "r");
	if (fp == NULL) { return ""; };

	/* Drain FP */
	ans = drainFP( fp );

	/* close and return data */
	fclose(fp);
	return ans;

}

/**
 * Get platform string
 */
string crashReportPlatformString() {

	// Build string
	string ans = "Unknown Linux";

	// Use lsb_release to get the short description
	string info = execAndGet("lsb_release -s -d");
	if (!info.empty()) ans = info;

	// Use uname to get kernel version
	info = openAndGet("/proc/version");
	if (!info.empty()) ans += " (" + info + ")";

	// Return platform string
	return ans;

}