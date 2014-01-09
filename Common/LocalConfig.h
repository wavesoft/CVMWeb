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
#ifndef CONFIG_H_4GP6DIPT
#define CONFIG_H_4GP6DIPT

#include "Utilities.h"
#include "CrashReport.h"
#include "ParameterMap.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

/**
 * Shared pointer for the LocalConfig class
 */
class LocalConfig;
typedef boost::shared_ptr< LocalConfig >                                LocalConfigPtr;

/**
 * LocalConfig is a subclass of ParameterMap that can be stored
 * to the local configuration slot.
 */
class LocalConfig : public ParameterMap
{
public:

    /**
     * Private constructor that initializes the LocalConfig Class.
     * THIS SHOULD NOT BE USED DIRECTLY
     *
     * If you want to create a new instance, you must use the LocalConfig::global(), LocalConfig::runtime() or the
     * LocalConfig::forRuntime( "name" ) functions.
     */
    LocalConfig ( std::string configDir, std::string configName );

    /**
     * Return a LocalConfig Shared Pointer for the global config
     */
    static      LocalConfigPtr  global();

    /**
     * Return a LocalConfig Shared Pointer for the runtime config
     */
    static      LocalConfigPtr  runtime();

    /**
     * Return a LocalConfig Shared Pointer for the specified runtime config
     */
    static      LocalConfigPtr  forRuntime( const std::string& name );

    /**
     * Upgrade a ParameterMap to a LocalConfig Shared Pointer for the specifeid runtime config
     */
    static      LocalConfigPtr  upgrade( ParameterMapPtr map, const std::string& name );

    /**
     * Populate the given vector with the lines from a config file with the given name
     */
    bool                        loadLines       ( std::string file, std::vector<std::string> * lines );

    /**
     * Populate the given string bufer from a config file with the given name
     */
    bool                        loadBuffer      ( std::string file, std::string * buffer );

    /**
     * Populate the given dictionary from a config file with the given name
     */
    bool                        loadMap         ( std::string file, std::map<std::string, std::string> * map );

    /**
     * Save the lines from the given buffer to a config file with the given name
     */
    bool                        saveLines       ( std::string file, std::vector<std::string> * lines );

    /**
     * Save the contents of the buffer to a config file with the given name
     */
    bool                        saveBuffer      ( std::string file, std::string * buffer );

    /**
     * Store the contents of the specified dictionary to a config file with the given name
     */
    bool                        saveMap         ( std::string file, std::map<std::string, std::string> * map );

    /**
     * Enumerate the names of the config files in the specified directory that matches the specified prefix.
     */
    std::vector< std::string >  enumFiles       ( std::string prefix = "" );

    /**
     * Return the modification time of the specified file
     */
    time_t                      getLastModified ( std::string configFile );

    /**
     * Check if the specified file name exists
     */
    bool                        exists          ( std::string configFile );

    /**
     * Return the full-path of the specified config file name
     */
    std::string                 getPath         ( std::string configFile );

    /**
     * Save the map to file
     */
    bool                        save            ( );

    /**
     * Load the map from file
     */
    bool                        load            ( );

    /**
     * Synchronize file contents with the disk
     */
    virtual bool                sync            ( );

    /**
     * Override the erase function so we can keep track of the 
     * changes done in the buffer.
     */
    virtual void                erase           ( const std::string& name );

    /**
     * Override the clear function so we can erase the file aswell
     */
    virtual void                clear            ( );

    /**
     * Override the clear function so we can erase the file aswell
     */
    virtual void                clearAll         ( );

    /**
     * Override the set function so we can keep track of the changes
     * done in the buffer.
     */
    virtual void                set             ( const std::string& name, std::string value );

private:

    /**
     * The base directory where to do the operations
     */
    std::string                 configDir;

    /**
     * The file name where the config variables are stored from
     */
    std::string                 configName;

    /**
     * Used by the global() function to implement singleton template
     */
    static LocalConfigPtr       globalConfigSingleton;

    /**
     * Used by the runtime() function to implement singleton template
     */
    static LocalConfigPtr       runtimeConfigSingleton;

    /**
     * The time the parameter map was loaded (for synchronization).
     */
    unsigned long long          timeLoaded;

    /**
     * The time the parameter map was last modified (and not commited).
     */
    unsigned long long          timeModified;

    /**
     * Parameters erased from file
     */
    std::list<std::string>      keysDeleted;

protected:
    
    /**
     * Overrided function from ParameterMap to commit changes to the file
     */
    virtual void                commitChanges   ( );

};


#endif /* end of include guard: CONFIG_H_4GP6DIPT */
