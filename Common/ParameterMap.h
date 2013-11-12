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

#include "CVMGlobals.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

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
	ParameterMap( ) : parent(), prefix(""), locked(false), changed(false) {

		// Allocate a new shared pointer
		parameters = boost::make_shared< std::map< std::string, std::string > >( );

	};

	/**
	 * Create a new parameter map with the specified parameters
	 */
	ParameterMap( ParameterDataMapPtr parametersptr, std::string pfx ) : parent(), parameters(parametersptr), prefix(pfx), locked(false), changed(false) 
		{ };


	/**
	 * Create a new parameter map by using the specified as parent
	 */
	ParameterMap( ParameterMapPtr parentptr, std::string pfx ) : parent(parentptr), prefix(pfx), locked(false), changed(false) {

		// Use the pointer from the parent class
		parameters = parentptr->parameters;

	};

	/** 
	 * Lock updates
	 */
	void 						lock 			( );

	/** 
	 * Unlock updates and commit
	 */
	void 						unlock 			( );

	/**
	 * Empty the parameter set
	 *
	 * This function will remove only the parameters under the
	 * current prefix.
	 */
	void 						clear			( );

	/**
	 * Clean the entire parameter set
	 *
	 * This function will remove all the parameters in the dictionary
	 * that will also affect subgroup children.
	 */
	void 						clearAll		( );

	/**
	 * Return a string parameter value
	 */
    std::string                 get             ( const std::string& name, std::string defaultValue = "" );

    /**
     * Set a string parameter
     */
    void                        set             ( const std::string& name, std::string value );

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
     * Return a sub-parameter group instance
     */
    ParameterMapPtr				subgroup		( const std::string& name );

    /**
     * Enumerate the variable names that match our current prefix
     */
    std::vector< std::string >	enumKeys    	( );

    /**
     * Update all the parameters from the specified map
     */
    void						fromMap			( std::map< std::string, std::string> * map, bool clearBefore = false );

    /**
     * Update all the parameters from the specified parameter map
     */
    void						fromParameters	( const ParameterMapPtr& ptr, bool clearBefore = false );

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
	 * Optional parent class
	 */
	ParameterMapPtr				parent;

	/**
	 * Flags if we are locked
	 */
	bool						locked;

	/**
	 * Flags if something was chaged between a locked() state
	 */
	bool 						changed;

protected:
	
	/**
	 * Locally overridable function to commit changes to the dictionary
	 */
	virtual void 				commitChanges	( );

};

#endif /* end of include guard: PARAMETERMAP_H */
