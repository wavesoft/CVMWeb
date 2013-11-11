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

#ifndef PARAMETERMAP_H
#define PARAMETERMAP_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

#include <string>
#include <map>
#include <vector>

/**
 * The prefix separator for groupping parameter names
 */
#define GROUP_PREFIX	"/"

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
	 * Create a new blank parameter map
	 */
	ParameterMap( ) : parent(), prefix("") {

		// Allocate a new shared pointer
		parameters = boost::make_shared< std::map< std::string, std::string > >( );

	};

	/**
	 * Create a new parameter map by using the specified as parent
	 */
	ParameterMap( ParameterMapPtr parentptr, std::string pfx ) : parent(parentptr), prefix(pfx) {

		// Use the pointer from the parent class
		parameters = parentptr->parameters;

	};

	/**
	 * Return a string parameter value
	 */
    std::string                 get             ( const std::string& name, std::string defaultValue = "" );

    /**
     * Set a string parameter
     */
    void                        set             ( const std::string& name, std::string value );

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
   	 * Overload bracket operator
   	 */
    std::string operator 		[]				(const std::string& i) const { return parameters->at(i); }
    std::string & operator 		[]				(const std::string& i) {return parameters->at(i);}

	/**
	 * Publically accessible parameter map
	 */
	ParameterDataMapPtr			parameters;

private:

	/**
	 * Prefix for all the parameters
	 */
	std::string					prefix;

	/**
	 * Optional parent class
	 */
	ParameterMapPtr				parent;

protected:
	
	/**
	 * Locally overridable function to commit changes to the dictionary
	 */
	virtual void 				commitChanges	( );

};

#endif /* end of include guard: PARAMETERMAP_H */
