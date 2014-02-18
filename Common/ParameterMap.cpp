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

#include "CrashReport.h"
#include "Utilities.h"
#include "ParameterMap.h"

/**
 * Allocate a new shared pointer
 */
ParameterMapPtr ParameterMap::instance ( ) {
    CRASH_REPORT_BEGIN;
    return boost::make_shared< ParameterMap >();
    CRASH_REPORT_END;
}

/**
 * Return a string parameter value
 */
std::string ParameterMap::get( const std::string& kname, std::string defaultValue ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    if (parameters->find(name) == parameters->end())
        return defaultValue;
    return (*parameters)[name];
    CRASH_REPORT_END;
}

/**
 * Set a string parameter
 */
ParameterMap& ParameterMap::set ( const std::string& kname, std::string value ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;
    (*parameters)[name] = value;
    if (!locked) {
        commitChanges();
    } else {
        changed = true;
    }
    return *this;
    CRASH_REPORT_END;
}

/**
 * Delete a parameter
 */
ParameterMap& ParameterMap::erase ( const std::string& name ) {
    CRASH_REPORT_BEGIN;
    std::map<std::string, std::string>::iterator e = parameters->find(prefix+name);
    if (e != parameters->end())
        parameters->erase(e);
    return *this;
    CRASH_REPORT_END;
}

/**
 * Set a string parameter only if the value is missing.
 * This does not trigger the commitChanges function.
 */
void ParameterMap::setDefault ( const std::string& kname, std::string value ) {
    CRASH_REPORT_BEGIN;
    std::string name = prefix + kname;

    // Insert the given entry (only if it's missing)
    // and don't trigger commitChanges.
    parameters->insert(std::pair< std::string, std::string >( name, value ));

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
    return ston<T>((*parameters)[name]);
    CRASH_REPORT_END;
}

/**
 * Set a numeric parameter value
 */
template<typename T> void ParameterMap::setNum ( const std::string& kname, T value ) {
    CRASH_REPORT_BEGIN;
    set( kname, ntos<T>(value) );
    CRASH_REPORT_END;
}

/**
 * Empty the parameter set
 */
ParameterMap& ParameterMap::clear( ) {
    CRASH_REPORT_BEGIN;

    // Get the keys for this group
    std::vector<std::string> myKeys = enumKeys();

    // Delete keys
    for (std::vector<std::string>::iterator it = myKeys.begin(); it != myKeys.end(); ++it) {
        parameters->erase( prefix + *it );
    }

    return *this;
    CRASH_REPORT_END;
}

/**
 * Empty all the parameter set
 */
ParameterMap& ParameterMap::clearAll( ) {
    CRASH_REPORT_BEGIN;
    parameters->clear();
    return *this;
    CRASH_REPORT_END;
}

/**
 * Lock the parameter map disable updates
 */
ParameterMap& ParameterMap::lock ( ) {
    CRASH_REPORT_BEGIN;
    locked = true;
    changed = false;
    return *this;
    CRASH_REPORT_END;
}

/**
 * Unlock the parameter map and enable updates
 */
ParameterMap& ParameterMap::unlock ( ) {
    CRASH_REPORT_BEGIN;
    locked = false;
    if (changed) commitChanges();
    return *this;
    CRASH_REPORT_END;
}

/**
 * Locally overridable function to commit changes to the dictionary
 */
void ParameterMap::commitChanges ( ) {
    CRASH_REPORT_BEGIN;

    // Forward event to parent
    if (parent) parent->commitChanges();
    
    CRASH_REPORT_END;
}

/**
 * Return a sub-parameter group instance
 */
ParameterMapPtr ParameterMap::subgroup( const std::string& kname ) {
    CRASH_REPORT_BEGIN;

    // Calculate the prefix of the sub-group
    std::string name = prefix + kname + PMAP_GROUP_SEPARATOR;

    // Return a new ParametersMap instance that encapsulate us
    // as parent.
    return boost::make_shared<ParameterMap>( shared_from_this(), name );

    CRASH_REPORT_END;
}

/**
 * Enumerate the variable names that match our current prefix
 */
std::vector< std::string > ParameterMap::enumKeys ( ) {
    CRASH_REPORT_BEGIN;

    std::vector< std::string > keys;

    // Loop over the entries in the record
    for ( std::map<std::string, std::string>::iterator it = parameters->begin(); it != parameters->end(); ++it ) {
        std::string key = (*it).first;

        // Check for matching prefix and lack of group separator in the rest of the key
        if ( (key.substr(0, prefix.length()).compare(prefix) == 0) && // Prefix Matches
             (key.substr(prefix.length(), key.length() - prefix.length() ).find(PMAP_GROUP_SEPARATOR) == std::string::npos) // No group separator found
            ) {

            // Store key name without prefix
            keys.push_back( key.substr(prefix.length(), key.length() - prefix.length()) ); 
        }

    }

    // Return the keys vector
    return keys;

    CRASH_REPORT_END;
}

/**
 * Return true if the specified parameter exists
 */
bool ParameterMap::contains ( const std::string& name, const bool useBlank ) {
    CRASH_REPORT_BEGIN;
    bool ans = (parameters->find(prefix + name) != parameters->end());
    if ( ans && useBlank ) {
        return !(*parameters)[prefix+name].empty();
    } else {
        return ans;
    }
    CRASH_REPORT_END;
}

/**
 * Update all the parameters from the specified map
 */
void ParameterMap::fromParameters ( const ParameterMapPtr& ptr, bool clearBefore ) {
    CRASH_REPORT_BEGIN;

    // Check if we have to clean the keys first
    if (clearBefore) clear();

    // Get parameter keys
    std::vector< std::string > ptrKeys = ptr->enumKeys();

    // Store values
    for (std::vector< std::string >::iterator it = ptrKeys.begin(); it != ptrKeys.end(); ++it) {
        CVMWA_LOG("INFO", "Importing key " << *it << " = " << ptr->parameters->at(*it));
        (*this->parameters)[prefix + *it] = (*ptr->parameters)[*it];
    }

    // If we are not locked, sync changes.
    // Oherwise mark us as dirty
    if (!locked) {
        commitChanges();
    } else {
        changed = true;
    }

    CRASH_REPORT_END;
}

/**
 * Update all the parameters from the specified map
 */
void ParameterMap::fromMap ( std::map< std::string, std::string> * map, bool clearBefore ) {
    CRASH_REPORT_BEGIN;

    // Check if we have to clean the keys first
    if (clearBefore) clear();

    // Store values
    if (map == NULL) return;
    for (std::map< std::string, std::string>::iterator it = map->begin(); it != map->end(); ++it) {
        (*parameters)[prefix + (*it).first] = (*it).second;
    }

    // If we are not locked, sync changes.
    // Oherwise mark us as dirty
    if (!locked) {
        commitChanges();
    } else {
        changed = true;
    }

    CRASH_REPORT_END;
}

/**
 * Update all the parameters from the specified JSON Value
 */
void ParameterMap::fromJSON( const Json::Value& json, bool clearBefore ){
    CRASH_REPORT_BEGIN;

    // Check if we have to clean the keys first
    if (clearBefore) clear();

    // Store values
    const Json::Value::Members membNames = json.getMemberNames();
    for (std::vector<std::string>::const_iterator it = membNames.begin(); it != membNames.end(); ++it) {
        std::string k = *it;
        Json::Value v = json[k];
        if (v.isObject()) {
            ParameterMapPtr sg = subgroup(k);
            sg->fromJSON(v);
        } else if (v.isString()) {
            (*parameters)[k] = v.asString();
        } else if (v.isInt()) {
            int vv = v.asInt();
            (*parameters)[k] = ntos<int>( vv );
        }
    }

    // If we are not locked, sync changes.
    // Oherwise mark us as dirty
    if (!locked) {
        commitChanges();
    } else {
        changed = true;
    }

    CRASH_REPORT_END;
}

/**
 * Store all the parameters to the specified map
 */
void ParameterMap::toMap ( std::map< std::string, std::string> * map, bool clearBefore ) {
    CRASH_REPORT_BEGIN;

    // Check if we have to clean the keys first
    if (clearBefore) map->clear();

    // Get the keys for this group
    std::vector<std::string> myKeys = enumKeys();

    // Store my keys to map
    for (std::vector<std::string>::iterator it = myKeys.begin(); it != myKeys.end(); ++it) {
        (*map)[*it] = (*parameters)[ prefix + *it];
    }

    CRASH_REPORT_END;
}

/**
 * Set a boolean parameter
 */
void ParameterMap::setBool ( const std::string& name, bool value ) {
    std::string v = "n";
    if (value) v = "y";
    set(name, v);
}

/**
 * Get a boolean parameter
 */
bool ParameterMap::getBool ( const std::string& name, bool defaultValue ) {
    std::string v = get(name, "");
    if (v.empty()) return defaultValue;
    return ((v[0] == 'y') || (v[0] == 't') || (v[0] == '1'));
}

/**
 * Synchronize file contents with the dictionary contents
 */
bool ParameterMap::sync ( ) {

    // If we have parent, forward to the root element
    if (parent) {
        return parent->sync();
    }

    // Otherwise succeed - we can't sync anything
    // one way or another.
    return true;

}

/**
 * Template implementations for numeric values
 */
template int ParameterMap::getNum<int>( const std::string&, int defValue );
template void ParameterMap::setNum<int>( const std::string&, int value );
template long ParameterMap::getNum<long>( const std::string&, long defValue );
template void ParameterMap::setNum<long>( const std::string&, long value );
