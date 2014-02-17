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

#pragma once
#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Global information
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * The plug-in version
 */
#define     CERNVM_WEBAPI_VERSION           "2.0"

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Common URLs
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * The hypervisor configuration URL (plugin version will be appended)
 */
#define		URL_HYPERVISOR_CONFIG			"http://cernvm.cern.ch/releases/webapi/hypervisor.config?ver="

/**
 * The base URL where the releases tree is located
 */
#define 	URL_CERNVM_RELEASES				"http://cernvm.cern.ch/releases"

/**
 * The URL of the trusted domain list
 */
#define 	URL_CRYPTO_STORE            	"http://cernvm.cern.ch/releases/webapi/keystore/domainkeys.lst"

/**
 * The URL of the signature of the trusted domain list
 */
#define 	URL_CRYPTO_SIGNATURE        	"http://cernvm.cern.ch/releases/webapi/keystore/domainkeys.sig"


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Crash reporting
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * The URL where the crash-reporting script is located
 */
#define 	CRASH_REPORT_URL            	"http://labs.wavesoft.gr/report/crash-report.php"

/**
 * The header of the crash-report log
 */
#define 	CRASH_REPORT_HEAD				"An exception occured in a user's CVMWebAPI plugin. The following report is produced for troubleshooting:"

/**
 * The number of log lines to preserve on memory for creating
 * crash reports.
 */
#define 	CRASH_LOG_SCROLLBACK			300


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Cryptographic parameters
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * Thresshold between consecutive requests (seconds)
 */
#define 	CRYPTO_FREQUENT_THRESSHOLD  	60

/**
 * The validity of the trusted domain list (seconds).
 * After this time the store will be flushed and re-downloaded.
 */
#define 	CRYPTO_STORE_VALIDITY    		86400

/**
 * Hard-coded salt used in some cryptographic functions
 */
#define		CRYPTO_SALT 					"_WSAStartup@8452"

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Default parameters for various places
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * Default CernVM Version 
 */
#define 	DEFAULT_CERNVM_VERSION  		"1.13-12"

/**
 * Default CernVM Flavor 
 */
#define 	DEFAULT_CERNVM_FLAVOR  			"prod"

/**
 * Default CernVM Architecture
 */
#define		DEFAULT_CERNVM_ARCH				"x86_64"

/**
 * Default API Port 
 */
#define 	DEFAULT_API_PORT        		80

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Session Configuration
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


/**
 * Delay between twon concecutive user denies after which his requests
 * will be ignored (in milliseconds).
 */
#define 	THROTTLE_TIMESPAN       		5000 

/** 
 * After how many denies the plugin will be blocked for the session
 */
#define 	THROTTLE_TRIES          		2

/**
 * Maximum delay (in milliseconds) between two consecutive errors within
 * which a new error will be counted excess and placed in the error loop
 */
#define 	SESSION_HEAL_THRESSHOLD			60000

/**
 * Maximum number of retries within thressold after which we will abandon
 * any further attemt for error healing.
 */
#define 	SESSION_HEAL_TRIES				2


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////
//// Misc configuration
////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/**
 * The TCP Ports where the daemon listens for incoming connections
 */
#define		DAEMON_PORT         			58740

/**
 * The prefix separator used for sub-group definitions on ParameterMap
 */
#define 	PMAP_GROUP_SEPARATOR			"/"


#endif /* End of include guard COMMON_CONFIG_H */