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
#ifndef PARAMETERMAP_H
#define PARAMETERMAP_H

#include "Config.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <json/json.h>

#include <string>
#include <map>
#include <vector>

/**
 * Shared pointer for the map instance
 */
class ParameterMap;
typedef boost::shared_ptr< ParameterMap >       						ParameterMapPtr;
typedef boost::shared_ptr< std::map< std::string, std::string > >       ParameterDataMapPtr;

/**
 * This is a generic parameter mapping class.
 *
 * It can be used to store key/value pairs of arbitrary type that are
 * automatically converted to string. The string map can then be stored
 * to a file or handled appropriately.
 */
class ParameterMap : public boost::enable_shared_from_this<ParameterMap> {
public:

	/**
	 * Shorthand function to create a new map
	 */
	static ParameterMapPtr 		instance		( );

	/**
	 * Create a new blank parameter map
	 */
	ParameterMap( ) : parameters(), prefix(""), parent(), locked(false), changed(false) {

		// Allocate a new shared pointer
		parameters = boost::make_shared< std::map< std::string, std::string > >( );

	};

	/**
	 * Create a new parameter map with the specified parameters
	 */
	ParameterMap( ParameterDataMapPtr parametersptr, std::string pfx ) : parameters(parametersptr), prefix(pfx), parent(), locked(false), changed(false) 
		{ };


	/**
	 * Create a new parameter map by using the specified as parent
	 */
	ParameterMap( ParameterMapPtr parentptr, std::string pfx ) : parameters(), prefix(pfx), parent(parentptr), locked(false), changed(false) {

		// Use the pointer from the parent class
		parameters = parentptr->parameters;

	};

	/**
	 * Empty the parameter set
	 *
	 * This function will remove only the parameters under the
	 * current prefix.
	 */
	virtual ParameterMap&		clear			( );

	/**
	 * Clean the entire parameter set
	 *
	 * This function will remove all the parameters in the dictionary
	 * that will also affect subgroup children.
	 */
	virtual ParameterMap&		clearAll		( );

	/**
	 * Return a string parameter value
	 */
    virtual std::string         get             ( const std::string& name, std::string defaultValue = "" );

    /**
     * Set a string parameter
     */
    virtual ParameterMap&		set             ( const std::string& name, std::string value );

    /**
     * Delete a parameter
     */
    virtual ParameterMap&		erase  			( const std::string& name );
    
    /**
     * Synchronize the contents with a possibly underlaying system
     */
    virtual bool 				sync 			( );

	/** 
	 * Lock updates
	 *
	 * You can use this function along with the unlock() function in order
	 * to optimize the write performance. lock() the parameter map before
	 * you set the parameter values and unlock() it when you are done.
	 */
	ParameterMap&				lock 			( );

	/** 
	 * Unlock updates and commit
	 *
	 * This function will synchronize the changes only if something has 
	 * changed since the time the lock() function was called.
	 */
	ParameterMap&				unlock 			( );

    /**
     * Set a string parameter only if there is no value already
     */
    void                        setDefault      ( const std::string& name, std::string value );

    /**
     * Get a numeric parameter value
     */
    template<typename T> T      getNum          ( const std::string& name, T defaultValue = (T)0 );

    /**
     * Set a numeric parameter value
     */
    template<typename T> void   setNum          ( const std::string& name, T value );

    /**
     * Set a boolean parameter
     */
    void                        setBool         ( const std::string& name, bool value );

    /**
     * Get a boolean parameter
     */
    bool                        getBool         ( const std::string& name, bool defaultValue = false );

    /**
     * Return a sub-parameter group instance
     */
    ParameterMapPtr				subgroup		( const std::string& name );

    /**
     * Enumerate the variable names that match our current prefix
     */
    std::vector< std::string >	enumKeys    	( );

    /**
     * Return true if the specified parameter exists
     */
    bool						contains 		( const std::string& name, const bool useBlank = false );

    /**
     * Update all the parameters from the specified map
     */
    void						fromMap			( std::map< std::string, std::string> * map, bool clearBefore = false );

    /**
     * Update all the parameters from the specified parameter map
     */
    void						fromParameters	( const ParameterMapPtr& ptr, bool clearBefore = false );

    /**
     * Update all the parameters from a JSON map
     */
    void						fromJSON		( const Json::Value& json, bool clearBefore = false );

    /**
     * Store all the parameters to the specified map
     */
    void						toMap			( std::map< std::string, std::string> * map, bool clearBefore = false );


   	/**
   	 * Overload bracket operator
   	 */
    std::string operator 		[]				(const std::string& i) const { return parameters->at(i); }
    std::string & operator 		[]				(const std::string& i) {return parameters->at(i);}

	/**
	 * The parameter map contents
	 */
	ParameterDataMapPtr			parameters;

	/**
	 * Prefix for all the parameters
	 */
	std::string					prefix;

private:

	/**
	 * Flags if we are locked
	 */
	bool						locked;

	/**
	 * Flags if something was chaged between a locked state
	 */
	bool 						changed;

protected:
	
	/**
	 * Optional parent class
	 */
	ParameterMapPtr				parent;

	/**
	 * Locally overridable function to commit changes to the dictionary
	 */
	virtual void 				commitChanges	( );

};

#endif /* end of include guard: PARAMETERMAP_H */
