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
#ifndef CVMUSERINTERACTION_H
#define CVMUSERINTERACTION_H

#include "Common/UserInteraction.h"

#include "JSAPIAuto.h"
#include "JSObject.h"
#include "variant_list.h"

/**
 * Forward declerations for CVMWebInteraction
 */
class CVMWebInteraction;
typedef boost::shared_ptr< CVMWebInteraction >	CVMWebInteractionPtr;

/**
 * A JSAPI object that provides a callback mechanism
 */
class CVMWebInteractionMessage : public FB::JSAPIAuto {
public:

    CVMWebInteractionMessage( const std::string& title, const std::string& content, const callbackResult& cb, const std::string& typeName
    						  const std::string& nameTitle = "title", const std::string& nameContent = "text" ) :
        resultCb(resultCb), title(title), content(content), typeName(typeName), JSAPIAuto("CVMWebInteractionMessage")
    {

    	// Register callback for reply
        registerMethod("reply",		       		make_method(this, &CVMWebInteractionMessage::send_reply ));

        // Read-only property
        registerProperty(nameTitle,           	make_property(this, &CVMWebInteractionMessage::get_title ));
        registerProperty(nameContent,  			make_property(this, &CVMWebInteractionMessage::get_content ));
        registerProperty("type", 				make_property(this, &CVMWebInteractionMessage::get_type ));

    }

    // Callbacks to return the variable contents
    std::string get_title() 		{ return title; };
    std::string get_content() 		{ return content; };
    std::string get_type() 			{ return typeName; };

    // Callback to call the result callback (wow?!)
    void send_reply( int result ) 	{ if (cb) cb(result); }

private:
	callbackResult		resultCb;
	std::string			title;
	std::string			content;
	std::string			typeName;

};

/**
 * User interaction that uses javascript I/O functions
 */
class CVMWebInteraction: public UserInteraction {
public:

	/**
	 * Initialize a web interaction that forward the events
	 * to the given javascript object
	 */
	static CVMWebInteractionPtr fromJSObject( const FB::variant &cb );

	/**
	 * Delegate function that forwards the request to the javascript interface
	 */
	void __callbackConfim		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackAlert		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackLicense		(const std::string&, const std::string&, const callbackResult& cb);
	void __callbackLicenseURL	(const std::string&, const std::string&, const callbackResult& cb);

private:

	/**
	 * Initialize a web interaction that forward the events
	 * to the given javascript object
	 */
	CVMWebInteraction 			( const FB::variant &cb );

	// The JSObject pointer for the javascript object we wrap
	FB::JSObjectPtr 			jsobject;

	// This flag is set to TRUE if the jsobject is valid
	bool						isAvailable;

};

#endif /* end of include guard: CVMUSERINTERACTION_H */
