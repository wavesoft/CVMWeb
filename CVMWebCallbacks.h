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
#ifndef H_CVMWEBCALLBACKS
#define H_CVMWEBCALLBACKS

#include "Common/Callbacks.h"

#include "JSObject.h"
#include "variant_list.h"

/**
 * Wrapper class that registers handlers for javascript events
 */
class FBCallbackHost {
public:

	FBCallbackHost( const CallbackHostPtr & ch ) : parent(ch) 	{ };

	void 				jsStarted( const FB::variant &cb )		{ cbStarted = cb };
	void				jsCompleted( const FB::variant &cb )	{ cbCompleted = cb };
	void 				jsFailed( const FB::variant &cb )		{ cbFailed = cb };
	void 				jsProgress( const FB::variant &cb )		{ cbProgress = cb };

private:

	const CallbackHostPtr parent;
	FB::variant 		jcbStarted;
	FB::variant 		jcbCompleted;
	FB::variant 		jcbFailed;
	FB::variant 		jcbProgress;

	void 				_delegate_jsStarted( const std::string & msg );
	void 				_delegate_jsCompleted( const std::string & msg );
	void 				_delegate_jsFailed( const std::string & msg, const int errorCode );
	void 				_delegate_jsProgress( const std::string& msg, const double progress );

};

#endif /* end of include guard: H_CVMWEBCALLBACKS */
