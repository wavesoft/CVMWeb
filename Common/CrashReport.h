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
#ifndef CRASH_REPORT_H
#define CRASH_REPORT_H

#include <iostream>
#include <sstream>

// If we don't have crash report enabled, register some dummy definitions
#ifndef CRASH_REPORTING
	#define CRASH_REPORT_BEGIN 	;
	#define CRASH_REPORT_END 	;
 	#define crashReportAddInfo(...) ;

#else

 	#include "Config.h"

	#include <string.h>
	#include <curl/curl.h>
	#include "Utilities.h"

	#if defined(_MSC_VER)

		/**
		 * If we are using VisualC Compiler, we don't have any other way 
		 * than using the StalkWalker to get the stack trace. 
		 */ 
		#include "Win/StackWalker.h"

		/**
		 * The CVMWebStackWalker subclass, renders all the output
		 * to a variable that can be then collected.
		 */
		class CVMWebStackWalker : public StackWalker {
		public:
		  					CVMWebStackWalker() : StackWalker(), stackTrace("") {}
                            CVMWebStackWalker( LPCSTR symbolsFile ) : StackWalker( 0x3D, symbolsFile ), stackTrace("") {}
		  					CVMWebStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess), stackTrace("") {}
			virtual void	OnOutput(LPCSTR szText) { 
		  						// Collect stack trace
		  						stackTrace += szText;
		  						// StackWalker echoes to debug console. That's ok...
		  						StackWalker::OnOutput(szText); 
		  					}
			std::string 	stackTrace;
		};

		// Also, pick a cross-compiler macro name for the current function name
		#define CVM_FUNC_NAME __FUNCTION__

	#elif defined(__GNUC__)

		/**
		 * On GCC compiler, we have the backtrace() function from <execinfo.h>
		 */
		#include <execinfo.h>

		// Also, pick a cross-compiler macro name for the current function name
		#define CVM_FUNC_NAME __func__

	#else

		/**
		 * No compiler defined. Nothing else to do... raise error
		 */
		#error No known stack walking technique is available for the compiler that you are using.

	#endif

	/* Initialization for the crash report subsystem */
	void 			crashReportInit();

    /* Load debug symbols for crash report */
    void            crashReportLoadSymbols( std::string binaryFileName );

	/* Add general information for the crash report */
	void 			crashReportAddInfo( std::string key, std::string value );

    /* Get platform version and description string */
    std::string     crashReportPlatformString();

    /* Cleanup for the crash report system */
	void 			crashReportCleanup();

	/* Register log entry to the crash report scroll-back buffer */
	void 			crashReportStoreLog( std::ostringstream & oss );

	/* Compiler-specific way of building the stack trace */
	std::string		crashReportBuildStackTrace();

	/* Send the error report */
	void 			crashSendReport( const char * function, const char * message, std::string stackTrace  );

	/**
	 * The macros that will go around every function
	 *
	 * Usage:
	 *
	 *  function() {
	 *		CRASH_REPORT_BEGIN
	 *			.. Throw exceptions here ..
	 *		CRASH_REPORT_END
	 *  }
	 *
	 */
	#define CRASH_REPORT_BEGIN try {
	#define CRASH_REPORT_END   } \
	 	catch (const std::exception& ex) { \
	 		crashSendReport( CVM_FUNC_NAME, ex.what(), crashReportBuildStackTrace() ); \
	 		throw; \
	 	} catch (const std::string& ex) { \
	 		crashSendReport( CVM_FUNC_NAME, ex.c_str(), crashReportBuildStackTrace() ); \
	 		throw; \
	 	} catch (...) { \
	 		crashSendReport( CVM_FUNC_NAME, "Unknown exception", crashReportBuildStackTrace() ); \
	 		throw; \
	 	}

	#endif

#endif