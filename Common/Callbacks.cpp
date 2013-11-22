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

#include "Callbacks.h"


/**
 * Register a callback that handles the 'started' event
 */
void CallbackHost::onStarted ( const cbStarted & cb ) {
	startedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'completed' event
 */
void CallbackHost::onCompleted ( const cbCompleted & cb ) {
	completedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'failed' event
 */
void CallbackHost::onFailed ( const cbFailed & cb ) {
	failedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'progress' event
 */
void CallbackHost::onProgress ( const cbProgress & cb ) {
	progressCallbacks.push_back(cb);
}

/**
 * Fire the 'started' event
 */
void CallbackHost::fireStarted( const std::string & msg ){
	for (std::vector< cbStarted >::iterator it = startedCallbacks.begin(); it != startedCallbacks.end(); ++it) {
		cbStarted cb = *it;
		try {
			if (cb) cb( msg );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while handling 'started' event")
		}
	}
}

/**
 * Fire the 'completed' event
 */
void CallbackHost::fireCompleted( const std::string & msg ){
	for (std::vector< cbCompleted >::iterator it = completedCallbacks.begin(); it != completedCallbacks.end(); ++it) {
		cbCompleted cb = *it;
		try {
			if (cb) cb( msg );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while handling 'completed' event")
		}
	}
}

/**
 * Fire the 'failed' event
 */
void CallbackHost::fireFailed( const std::string & msg, const int errorCode ){
	for (std::vector< cbFailed >::iterator it = failedCallbacks.begin(); it != failedCallbacks.end(); ++it) {
		cbFailed cb = *it;
		try {
			if (cb) cb( msg, errorCode );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while handling 'failed' event")
		}
	}
}

/**
 * Fire the 'progress' event
 */
void CallbackHost::fireProgress( const std::string& msg, const double progress ) {
	for (std::vector< cbProgress >::iterator it = progressCallbacks.begin(); it != progressCallbacks.end(); ++it) {
		cbProgress cb = *it;
		try {
			if (cb) cb( msg, progress );
		} catch (...) {
			CVMWA_LOG("Error", "Exception while handling 'progress' event")
		}
	}
}

// Build the firebreath interface only if we are building for firebrath
#ifdef FIREBRATH_BUILD

/**
 * Register a javascript callback to handle the 'started' event
 */
void FBCallbackHost::jsStarted( const FB::variant &cb )	{ 
	if (IS_CB_AVAILABLE(cb)) {
		jcbStarted = cb;
		parent->onStarted( boost::bind( &FBCallbackHost::_delegate_jsStarted, _1 ) )
	}
};

/**
 * Register a javascript callback to handle the 'completed' event
 */
void FBCallbackHost::jsCompleted( const FB::variant &cb ) { 
	if (IS_CB_AVAILABLE(cb)) {
		jcbCompleted = cb;
		parent->onCompleted( boost::bind( &FBCallbackHost::_delegate_jsCompleted, _1 ) )
	}
};

/**
 * Register a javascript callback to handle the 'failed' event
 */
void FBCallbackHost::jsFailed( const FB::variant &cb ) {
	if (IS_CB_AVAILABLE(cb)) {
		jcbFailed = cb;
		parent->onFailure( boost::bind( &FBCallbackHost::_delegate_jsFailed, _1, _2 ) )
	}
};

/**
 * Register a javascript callback to handle the 'progress' event
 */
void FBCallbackHost::jsProgress( const FB::variant &cb ){ 
	if (IS_CB_AVAILABLE(cb)) {
		jcbProgress = cb;
		parent->onStarted( boost::bind( &FBCallbackHost::_delegate_jsProgress, _1, _2 ) )
	}
};

/**
 * Delegate function to forward the 'started' event to javascript
 */
void FBCallbackHost::_delegate_jsStarted( const std::string & msg ) {
	IS_CB_AVAILABLE(jcbStarted)
		jcbStarted.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( msg ));
}

/**
 * Delegate function to forward the 'completed' event to javascript
 */
void FBCallbackHost::_delegate_jsCompleted( const std::string & msg ) {
	IS_CB_AVAILABLE(jcbCompleted)
		jcbCompleted.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( msg ));
}

/**
 * Delegate function to forward the 'failed' event to javascript
 */
void FBCallbackHost::_delegate_jsFailed( const std::string & msg, const int errorCode ) {
	IS_CB_AVAILABLE(jcbFailed)
		jcbFailed.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( msg, errorCode ));
}

/**
 * Delegate function to forward the 'progress' event to javascript
 */
void FBCallbackHost::_delegate_jsProgress( const std::string& msg, const double progress ) {
	IS_CB_AVAILABLE(jcbProgress)
		jcbProgress.cast<FB::JSObjectPtr>()->InvokeAsync("", FB::variant_list_of( msg, progress ));
}



#endif
