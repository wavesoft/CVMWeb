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

#include "CVMWebCallbacks.h"

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
