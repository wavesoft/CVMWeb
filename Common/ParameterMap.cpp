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

#include "Common/CrashReport.h"
#include "Common/Utilities.h"
#include "ParameterMap.h"

/**
 * Return a string parameter value
 */
std::string ParameterMap::get( const std::string& kname, std::string defaultValue ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    if (parameters->find(name) == parameters->end())
        return defaultValue;
    return parameters->at(name);
    CRASH_REPORT_END;
}

/**
 * Set a string parameter
 */
void ParameterMap::set ( const std::string& kname, std::string value ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    parameters->insert(std::pair< std::string, std::string >( name, value ));
    commitChanges();
    CRASH_REPORT_END;
}

/**
 * Get a numeric parameter value
 */
template<typename T> T ParameterMap::getNum ( const std::string& kname, T defaultValue ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    if (parameters->find(name) == parameters->end())
        return defaultValue;
    return ston<T>(parameters->at(name));
    CRASH_REPORT_END;
}

/**
 * Set a numeric parameter value
 */
template<typename T> void ParameterMap::setNum ( const std::string& kname, T value ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    parameters->insert(std::pair< std::string, std::string >( name, ntos<T>( value ) ));
    commitChanges();
    CRASH_REPORT_END;
}

/**
 * Locally overridable function to commit changes to the dictionary
 */
void ParameterMap::commitChanges ( ) {

    // Forward event to parent
    if (parent) parent->commitChanges();
    
}

/**
 * Return a sub-parameter group instance
 */
ParameterMapPtr ParameterMap::subgroup( const std::string& kname ) {

    // Calculate the prefix of the sub-group
    std::string name = prefix + kname + GROUP_PREFIX;

    // Return a new ParametersMap instance that encapsulate us
    // as parent.
    return boost::make_shared<ParameterMap>( shared_from_this(), name );

}

/**
 * Enumerate the variable names that match our current prefix
 */
std::vector< std::string > ParameterMap::enumKeys ( ) {
    std::vector< std::string > keys;

    // Loop over the entries in the record
    for ( std::map<std::string, std::string>::iterator it = parameters->begin(); it != parameters->end(); ++it ) {
        std::string key = (*it).first;

        // Check for matching prefix and lack of group separator in the rest of the key
        if ( (key.substr(0, prefix.length()).compare(prefix) == 0) && // Prefix Matches
             (key.substr(prefix.length(), key.length() - prefix.length() ).find(GROUP_PREFIX) == std::string::npos) // No group separator found
            ) {

            // Store key name without prefix
            keys.push_back( key.substr(prefix.length(), key.length() - prefix.length()) ); 
        }

    }

    // Return the keys vector
    return keys;

}

/**
 * Template implementations for numeric values
 */
template int ParameterMap::getNum<int>( const std::string&, int defValue );
template void ParameterMap::setNum<int>( const std::string&, int value );
template long ParameterMap::getNum<long>( const std::string&, long defValue );
template void ParameterMap::setNum<long>( const std::string&, long value );
