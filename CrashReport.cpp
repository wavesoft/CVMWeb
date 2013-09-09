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

/* Initialization for the crash report subsystem */
void crashReportInit() {
	scrollBackPosition = 0;
}

/* Cleanup for the crash report system */
void crashReportCleanup() {

}

/* Add general information for the crash report */
void crashReportAddInfo( std::string key, std::string value ) {
	crashReportInfo[key] = value;
}

/* Register log entry to the crash report scroll-back buffer */
void crashReportStoreLog( ostringstream oss ) {

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
	CVMWebStackWalker mWalker;
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
 * Build the payload for the mail
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
	struct curl_slist *recipients = NULL;

	// Get current time in GMT
	time_t rawtime;
	struct tm * timeinfo;
	char timeBuffer[80];
	time (&rawtime);
	timeinfo = gmtime ( &rawtime );
	strftime(timeBuffer, 80, "%a, %d %b %Y %H:%M:%S %z", timeinfo);

	// Build random ID
	char idBuffer[46];
    for (int i = 0; i < 45; ++i) {
        idBuffer[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    idBuffer[45] = 0;

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
	oss << "Date: " << timeBuffer << "\n" <<
  		   "To: " CRASH_REPORT_TO "\n" <<
  		   "From: " CRASH_REPORT_FROM "\n" <<
  		   "Message-ID: <" << idBuffer << "@" CRASH_REPORT_RFCID ">\n" <<
  		   "Subject: " CRASH_REPORT_SUBJECT "\n" <<
  		   "\n" << /* empty line to divide headers from body, see RFC5322 */ 

  		   CRASH_REPORT_HEAD "\n\n" <<

  		   "-----------------------\n" <<
  		   " General information\n" <<
  		   "-----------------------\n" <<
  		   "Date: " << timeBuffer << "\n" <<
  		   "Function: " << function << "\n" <<
  		   "Exception message: " << message << "\n" <<
  		   generalInfo <<

  		   "\n" <<
  		   "-----------------------\n" <<
  		   " Latest error log entries\n" <<
  		   "-----------------------\n" <<
  		   backlog <<

  		   "\n" <<
  		   "-----------------------\n" <<
  		   " Stack trace\n" <<
  		   "-----------------------\n" <<
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
		/* This is the URL for your mailserver. Note the use of port 587 here,
		 * instead of the normal SMTP port (25). Port 587 is commonly used for
		 * secure mail submission (see RFC4403), but you should use whatever
		 * matches your server configuration. */ 
		curl_easy_setopt(curl, CURLOPT_URL, CRASH_REPORT_SMTP );

		/* In this example, we'll start with a plain text connection, and upgrade
		 * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
		 * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
		 * will continue anyway - see the security discussion in the libcurl
		 * tutorial for more details. */ 
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

		/* If your server doesn't have a valid certificate, then you can disable
		 * part of the Transport Layer Security protection by setting the
		 * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false).
		 *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		 *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		 * That is, in general, a bad idea. It is still better than sending your
		 * authentication details in plain text though.
		 * Instead, you should get the issuer certificate (or the host certificate
		 * if the certificate is self-signed) and add it to the set of certificates
		 * that are known to libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See
		 * docs/SSLCERTS for more information.
		 */ 
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		/* A common reason for requiring transport security is to protect
		 * authentication details (user names and passwords) from being "snooped"
		 * on the network. Here is how the user name and password are provided: */ 
		#if defined(CRASH_REPORT_SMTP_PASSWORD) && defined(CRASH_REPORT_SMTP_LOGIN)
		curl_easy_setopt(curl, CURLOPT_USERNAME, CRASH_REPORT_SMTP_LOGIN);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, CRASH_REPORT_SMTP_PASSWORD);
		#endif

		/* value for envelope reverse-path */ 
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, CRASH_REPORT_FROM);
		/* Add two recipients, in this particular case they correspond to the
		 * To: and Cc: addressees in the header, but they could be any kind of
		 * recipient. */ 
		recipients = curl_slist_append(recipients, CRASH_REPORT_TO);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		/* In this case, we're using a callback function to specify the data. You
		 * could just use the CURLOPT_READDATA option to specify a FILE pointer to
		 * read from.
		 */ 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);

		/* Since the traffic will be encrypted, it is very useful to turn on debug
		 * information within libcurl to see what is happening during the transfer.
		 */ 
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* send the message (including headers) */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
		  fprintf(stderr, "curl_easy_perform() failed: %s\n",
		          curl_easy_strerror(res));

		/* free the list of recipients and clean up */ 
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
	}

}

#endif