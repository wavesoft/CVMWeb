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

#include "CrashReport.h"
#include <time.h>
using namespace std;

// If we don't have crash reporting enabled, produce no code
#ifdef CRASH_REPORTING


/* Upload status for cURL */
struct upload_context {
	const char * 	data;
	size_t 			datalen;
	size_t 			datasent;
};

/* Scrollback buffer for the crashReport */
string 				scrollbackBuffer[ CRASH_LOG_SCROLLBACK ];
int 				scrollBackPosition = 0;
map<string, string>	crashReportInfo;

/* We are using custom debug symbols file on VC++ */
#if defined(_MSC_VER)
string              crashReportDebugSymbolsFile;
#endif

/* Initialization for the crash report subsystem */
void crashReportInit( string binaryFileName ) {
	scrollBackPosition = 0;

#if defined(_MSC_VER)
    // In Visual C++ we have to load the PDF file in order to resolve the symbols
    crashReportDebugSymbolsFile = binaryFileName.substr(0, binaryFileName.length() - 3) + "pdb";
#endif
}

/* Cleanup for the crash report system */
void crashReportCleanup() {

}

/* Add general information for the crash report */
void crashReportAddInfo( std::string key, std::string value ) {
	crashReportInfo[key] = value;
}

/* Register log entry to the crash report scroll-back buffer */
void crashReportStoreLog( ostringstream & oss ) {

	// Until we reach the buffer size, stack entries
	if (scrollBackPosition < CRASH_LOG_SCROLLBACK) {
		scrollbackBuffer[scrollBackPosition] = oss.str();
		scrollBackPosition++;

	// Otherwise, rotate upwards
	} else {
		for (int i=0; i<CRASH_LOG_SCROLLBACK-1; i++) {
			scrollbackBuffer[i] = scrollbackBuffer[i+1];
		}
		scrollbackBuffer[CRASH_LOG_SCROLLBACK-1] = oss.str();
	}

}

/**
 * Pretty-print the stack trace to the response string given
 */
std::string crashReportBuildStackTrace() {

	// Allocate space
	string cBuffer = "";

#if defined(_MSC_VER)
	// Use StalkWalker on windows
    CVMWebStackWalker mWalker( crashReportDebugSymbolsFile.c_str() );
	mWalker.ShowCallstack();
	cBuffer = mWalker.stackTrace;

#elif defined(__GNUC__)
	void *array[100];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 100);

	// Render to strings
	char** lines = backtrace_symbols(array, size);
	for (size_t i=0; i<size; i++) {
		cBuffer += lines[i];
	}
	free(lines);

#endif

	// Return response
	return cBuffer;

}
 
/**
 * Build the payload for the PUT request
 */
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct upload_context *upload_ctx = (struct upload_context *)userp;
	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}

	// Check how many data are remaining
	size_t txsize = upload_ctx->datalen - upload_ctx->datasent;

	// If they are more than the buffer size, chop them
	if (txsize > size) txsize = size;

	// Send
	memcpy(ptr, upload_ctx->data + upload_ctx->datasent, txsize);
	upload_ctx->datasent += txsize;

	// Return the number of bytes sent
	return txsize;
	
}

/**
 * Transmit the crash report
 */
void crashSendReport( const char * function, const char * message, std::string stackTrace ) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
 
	CURL *curl;
	CURLcode res;

	// Get current time in GMT
	char * timeBuffer = getTimestamp();

    // Build backlog
    string backlog = "";
    for (int i=0; i<scrollBackPosition; i++) {
    	backlog += scrollbackBuffer[i];
    }

    // Build general info
    string generalInfo;
    for (std::map<string, string>::iterator it=crashReportInfo.begin(); it!=crashReportInfo.end(); ++it) {
        string k = (*it).first;
        string v = (*it).second;
        generalInfo += k + ": " + v + "\n";
	}    

	// Build mail
	ostringstream oss;
	oss <<  // (0) Head message
            CRASH_REPORT_HEAD "\n\n" <<
            
            // (1) General info
            "Timestamp: " << timeBuffer << "\n" <<
  		    "Function: " << function << "\n" <<
  		    "Exception message: " << message << "\n" << 
            "Scrollback size: " << CRASH_LOG_SCROLLBACK << "\n" <<
            generalInfo << "\n" <<

            // (2) Backlog
  		    backlog << "\n\n" <<

            // (3) Stack trace
  		    stackTrace;


  	// Store the data to be sent on the upload context
	struct upload_context upload_ctx;
  	string data = oss.str();
  	upload_ctx.data = data.c_str();
  	upload_ctx.datalen = data.length();
  	upload_ctx.datasent = 0;

	// Setup CURL and send mail
	curl = curl_easy_init();
	if (curl) {

        // Setup the read data callback
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);

        // Enable uploading
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        // Use HTTP PUT 
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);

        // Target URL
        curl_easy_setopt(curl, CURLOPT_URL, CRASH_REPORT_URL);

        // Set file size
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)data.length());

		// send the message
		res = curl_easy_perform(curl);

		// Check for errors
		if(res != CURLE_OK)
		    CVMWA_LOG("Error", "curl_easy_perform() failed:" << curl_easy_strerror(res));

		// Cleanup CURL
		curl_easy_cleanup(curl);

	}

}

#endif