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
 * Wrapper class that forawrds events to a javascript object callback
 *
 * The javascript object automatically binds to named Callback events, using
 * the 'on-' callback. For example:
 *
 * {
 *     onBegin: function( msg ) {
 *			...
 *     },
 *     onProgress: function( msg, v ) {
 *			...
 *     }
 * }
 *
 */
class JSObjectCallbacks {
public:

	JSObjectCallbacks 	( const Callbacks & ch, const FB::variant &cb );
	~JSObjectCallbacks	( );

private:

	const Callbacks 	parent;
	FB::JSObjectPtr 	jsobject;
	bool				isAvailable;
	cbAnyEvent			delegateCallback;

	/**
	 * Delegate function that forwards the events to the javascript object
	 */
	void 				_delegate_anyEvent( const std::string& msg, VariantArgList& args );

};

/**
 * Utility function to convert a VariantArgList vector to FB::VariantList vector
 */
FB::VariantList 		ArgVar2FBVar ( VariantArgList& argVariants );

#endif /* end of include guard: H_CVMWEBCALLBACKS */
