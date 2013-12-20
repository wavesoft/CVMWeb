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
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <map>
#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/variant.hpp>

#include "Utilities.h"

//////////////////////////////////////
// Named callbacks definition
//////////////////////////////////////

/* Typedef for variant callbacks */
typedef boost::variant< float, double, int, std::string >								VariantArg;
typedef std::vector< VariantArg >														VariantArgList;
typedef boost::function<void ( const std::string& event, VariantArgList& args )>		cbAnyEvent;
typedef boost::function<void ( VariantArgList& args )>									cbNamedEvent;

//////////////////////////////////////
// Classes and structures
//////////////////////////////////////

/**
 * Helper class for building callback arguments
 */
class ArgumentList {
public:
	ArgumentList( ) : args() { };
	ArgumentList( VariantArg arg ) : args(1,arg) { };
	ArgumentList& operator()( VariantArg arg ) {
		args.push_back(arg);
		return *this;
	}
	operator VariantArgList& () {
		return args;
	}
private:
	VariantArgList 		args;

};

/**
 * Helper class for looking up the appropriate function to
 * disconnect an anyEvent callback
 */
class AnyEventSlot {
public:
    AnyEventSlot( cbAnyEvent cb ): callback(cb) { };
	cbAnyEvent 		callback;
};
typedef boost::shared_ptr< AnyEventSlot >	AnyEventSlotPtr;

/**
 * Helper class for looking up the appropriate function to
 * disconnect a namedEvent callback
 */
class NamedEventSlot {
public:
    NamedEventSlot( cbNamedEvent cb ): callback(cb) { };
	cbNamedEvent 	callback;
};
typedef boost::shared_ptr< NamedEventSlot >	NamedEventSlotPtr;


/**
 * The Callbacks class provides the interface to register and fire
 * callbacks by name.
 */
class Callbacks {
public:

	Callbacks() : anyEventCallbacks(), namedEventCallbacks() { };

	/**
	 * Register a callback that will be fired for all events
	 */
	AnyEventSlotPtr		onAnyEvent	( cbAnyEvent cb );

	/**
	 * Unregister a callback that will be fired for all events
	 */
	void 				offAnyEvent	( AnyEventSlotPtr cb );

	/**
	 * Register a callback that will be fired when the specific event occurs
	 */
	NamedEventSlotPtr	on 			( const std::string& name, cbNamedEvent cb );

	/**
	 * Unregister a callback that will be fired when the specific event occurs
	 */
	void 				off 		( const std::string& name, NamedEventSlotPtr cb );

	 /**
	  * Fire a named event
	  */
	 void 				fire 		( const std::string& name, VariantArgList& args );

public:

	// Callback list
	std::vector< AnyEventSlotPtr >								anyEventCallbacks;
	std::map< std::string, std::vector< NamedEventSlotPtr > > 	namedEventCallbacks;

};

#endif /* end of include guard: CALLBACKS_H */