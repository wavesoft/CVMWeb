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

#include <boost/filesystem.hpp> 

#include "Hypervisor.h"
#include "LocalConfig.h"

// Initialize singletons
LocalConfigPtr LocalConfig::globalConfigSingleton;
LocalConfigPtr LocalConfig::runtimeConfigSingleton;

/**
 * Return a LocalConfig Shared Pointer for the global config
 */
LocalConfigPtr LocalConfig::global() {
    CRASH_REPORT_BEGIN;

    // Ensure we have signeton
    if (!LocalConfig::globalConfigSingleton) {
        LocalConfig::globalConfigSingleton = boost::make_shared< LocalConfig >( getAppDataPath() + "/config", "global" );
    }

    // Return reference
    return LocalConfig::globalConfigSingleton;

    CRASH_REPORT_END;
}

/**
 * Return a LocalConfig Shared Pointer for the runtime config
 */
LocalConfigPtr LocalConfig::runtime() {
    CRASH_REPORT_BEGIN;

    // Ensure we have signeton
    if (!LocalConfig::runtimeConfigSingleton) {
        LocalConfig::runtimeConfigSingleton = boost::make_shared< LocalConfig >( getAppDataPath() + "/run", "runtime" );
    }

    // Return reference
    return LocalConfig::runtimeConfigSingleton;

    CRASH_REPORT_END;
}

/**
 * Return a LocalConfig Shared Pointer for the specified runtime config
 */
LocalConfigPtr LocalConfig::forRuntime( const std::string& name ) {
    CRASH_REPORT_BEGIN;

    // Return a shared pointer instance
    return boost::make_shared< LocalConfig >( getAppDataPath() + "/run", name );
    
    CRASH_REPORT_END;
}

/**
 * Create custom configuration file from the given map file
 */
LocalConfig::LocalConfig ( std::string path, std::string name ) : ParameterMap(), keysDeleted(), timeLoaded(0), timeModified(0) {
    CRASH_REPORT_BEGIN;

    // Prepare names
    this->configDir = path;
    this->configName = name;

    // Load parameters in the parameters map
    this->loadMap( name, parameters.get() );

    // Update time it was loaded and modified
    timeLoaded = getTimeInMs();
    timeModified = getTimeInMs();

    CRASH_REPORT_END;
}

/**
 * Enumerate the names of the config files in the specified directory that matches the specified prefix.
 */
std::vector< std::string > LocalConfig::enumFiles ( std::string prefix ) {
    CRASH_REPORT_BEGIN;

    boost::filesystem::path configDir( this->configDir );
    boost::filesystem::directory_iterator end_iter;

    std::vector< std::string > result;

    // Start scanning configuration directory
    if ( boost::filesystem::exists(configDir) && boost::filesystem::is_directory(configDir)) {
        for( boost::filesystem::directory_iterator dir_iter(configDir) ; dir_iter != end_iter ; ++dir_iter) {
            if (boost::filesystem::is_regular_file(dir_iter->status()) ) {

                // Get the filename
                std::string fn = dir_iter->path().filename().string();

                // Check if prefix and suffix matches
                if ( ( (prefix.empty()) || (fn.substr(0, prefix.length()).compare(prefix) == 0 ) ) &&
                     ( fn.substr(fn.length()-5, 5).compare(".conf") == 0 )   ) {

                    // Return name
                    result.push_back( fn.substr(0, fn.length()-5) );

                }
            }
        }
    }

    // Return results
    return result;
    CRASH_REPORT_END;
}

/**
 * Save all the lines to the given list file
 */
bool LocalConfig::saveLines ( std::string name, std::vector<std::string> * lines ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single instance can write at a time
    std::string file = systemPath(this->configDir + "/" + name + ".lst");
    NAMED_MUTEX_LOCK(file);

    // Truncate file
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) return false;
    
    // Dump the contents
    for (std::vector<std::string>::iterator i = lines->begin(); i != lines->end(); ++i) {
        std::string line = *i;

        // Store line
        ofs << line << std::endl;
    }
    
    // Close
    ofs.close();
    return true;

    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Save the string buffer to the given data file
 */
bool LocalConfig::saveBuffer ( std::string name, std::string * buffer ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single isntance can access the file
    std::string file = systemPath(this->configDir + "/" + name + ".dat");
    NAMED_MUTEX_LOCK(file);

    // Truncate file
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) return false;
    
    // Dump the contents
    ofs << buffer;
    
    // Close
    ofs.close();
    return true;

    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}
/**
 * Save string map to the given config file
 */
bool LocalConfig::saveMap ( std::string name, std::map<std::string, std::string> * map ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single isntance can access the file
    std::string file = systemPath(this->configDir + "/" + name + ".conf");
    NAMED_MUTEX_LOCK(file);
    CVMWA_LOG("Config", "OPEN Saving " << file );

    // Truncate file
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) {
        CVMWA_LOG("Error", "SaveMap failed while oppening " << file );
        ofs.close();
        return false;
    }
    
    // Dump the contents
    std::string::size_type pos = 0;
    for (std::map<std::string, std::string>::iterator it=map->begin(); it!=map->end(); ++it) {
        std::string key = (*it).first;
        std::string value = (*it).second;

        // Do not allow new-line span: Replace \n to "\n", \r to "\r" and "\" to "\\"
        pos = 0;
        while((pos = value.find("\\", pos)) != std::string::npos) {
            value.replace(pos, 1, "\\\\");
            pos += 2;
        }
        pos = 0;
        while((pos = value.find("\n", pos)) != std::string::npos) {
            value.replace(pos, 1, "\\n");
            pos += 2;
        }
        pos = 0;
        while((pos = value.find("\r", pos)) != std::string::npos) {
            value.replace(pos, 2, "\\r");
            pos += 2;
        }

        ofs << key << "=" << value << std::endl;
    }
    
    // Close
    ofs.flush();
    ofs.close();

    CVMWA_LOG("Config", "CLOSE Closing " << file );

    return true;
    
    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Load all the lines from the given list file
 */
bool LocalConfig::loadLines ( std::string name, std::vector<std::string> * lines ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single isntance can access the file
    std::string file = systemPath(this->configDir + "/" + name + ".lst");
    NAMED_MUTEX_LOCK(file);

    // Load configuration
    std::ifstream ifs ( file.c_str() , std::ifstream::in);
    if (ifs.fail()) return false;
    
    // Read file
    std::string line;
    lines->clear();
    while( std::getline(ifs, line) ) {

        // Store line
        lines->push_back(line);

    }
    
    // Close file
    ifs.close();
    return true;
    
    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Read the string buffer from the given data file
 */
bool LocalConfig::loadBuffer ( std::string name, std::string * buffer ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single isntance can access the file
    std::string file = systemPath(this->configDir + "/" + name + ".dat");
    NAMED_MUTEX_LOCK(file);

    // Load configuration
    std::ifstream ifs ( file.c_str() , std::ifstream::in);
    if (ifs.fail()) return false;
    
    // Read file
    *buffer = "";
    std::string line;
    while( std::getline(ifs, line) ) {
        *buffer += line + "\n";
    }
    
    // Close file
    ifs.close();
    return true;
    
    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Load string map from the given config file
 */
bool LocalConfig::loadMap ( std::string name, std::map<std::string, std::string> * map ) {
    CRASH_REPORT_BEGIN;
    
    // Only a single isntance can access the file
    std::string file = systemPath(this->configDir + "/" + name + ".conf");
    NAMED_MUTEX_LOCK(file);
    CVMWA_LOG( "Config", "OPEN LoadingMap " << file.c_str()  );

    // Load configuration
    std::ifstream ifs ( file.c_str() , std::ifstream::in);
    if (ifs.fail()) {
        CVMWA_LOG("Error", "Error loading map from " << file );
        ifs.close();
        return false;
    }
    
    // Read file
    std::string line;
    std::string::size_type pos = 0;
    map->clear();
    while( std::getline(ifs, line) ) {
        std::istringstream is_line(line);
        std::string key;
        if( std::getline(is_line, key, '=') ) {
            std::string value;
            if( std::getline(is_line, value) ) {

                // Revert new-line span: Replace "\n" to \n, "\r" to \r and "\\"" to "\"
                pos = 0;
                while((pos = value.find("\\\\", pos)) != std::string::npos) {
                    value.replace(pos, 2, "\\");
                    pos += 1;
                }
                pos = 0;
                while((pos = value.find("\\n", pos)) != std::string::npos) {
                    value.replace(pos, 2, "\n");
                    pos += 1;
                }
                pos = 0;
                while((pos = value.find("\\r", pos)) != std::string::npos) {
                    value.replace(pos, 2, "\r");
                    pos += 1;
                }

                // Insert into map
                map->insert( std::pair<std::string,std::string>(key, value) );

            }
        }
    }
    
    // Close file
    ifs.close();
    CVMWA_LOG("Config", "CLOSE Closing " << file );
    return true;
    
    NAMED_MUTEX_UNLOCK;
    CRASH_REPORT_END;
}

/**
 * Create a full path from the given config file name
 */
std::string LocalConfig::getPath( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = systemPath(this->configDir + "/" + configFile);
    return file;
    CRASH_REPORT_END;
}

/**
 * Check if the specified config file exists
 */
bool LocalConfig::exists ( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = systemPath(this->configDir + "/" + configFile);
    return file_exists( file );
    CRASH_REPORT_END;
}

/**
 * Get the time the file was last modified
 */
time_t LocalConfig::getLastModified ( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = systemPath(this->configDir + "/" + configFile);

    // Get file modification time
    #ifdef _WIN32
    struct _stat attrib;
    _stat( file.c_str(), &attrib);
    #else
    struct stat attrib;
    stat( file.c_str(), &attrib);
    #endif
    
    // Convert it to time_t
    return attrib.st_mtime;
    
    CRASH_REPORT_END;
}

/**
 * Override the erase function so we can keep track of the 
 * changes done in the buffer.
 */
ParameterMap& LocalConfig::erase ( const std::string& name ) {
    CRASH_REPORT_BEGIN;

    // Update time modified
    timeModified = getTimeInMs();

    // Erase key
    ParameterMap& ans = ParameterMap::erase(name);

    // Store it on 'deleted keys'
    if (std::find(keysDeleted.begin(), keysDeleted.end(), prefix+name) == keysDeleted.end())
        keysDeleted.push_back(prefix + name);

    return ans;
    CRASH_REPORT_END;
}

/**
 * Override the clear function so we can remove the underlaying file aswell.
 */
ParameterMap& LocalConfig::clear ( ) {
    CRASH_REPORT_BEGIN;

    // Update time modified
    timeModified = getTimeInMs();

    // Clear all keys
    ParameterMap& ans = ParameterMap::clear();

    // If we don't have a prefix, we just did a 'clearAll'
    // Remove the file as well.
    if (prefix.empty()) {
        if (!parent) {
            std::string fName = systemPath(this->configDir + "/" + configName + ".conf");
            if (file_exists(fName))
                remove( fName.c_str() );
        }
    }

    return ans;
    CRASH_REPORT_END;
}

/**
 * Override the clearAll function so we can remove the underlaying file aswell.
 */
ParameterMap& LocalConfig::clearAll ( ) {
    CRASH_REPORT_BEGIN;

    // Update time modified
    timeModified = getTimeInMs();

    // Clear all keys
    ParameterMap& ans = ParameterMap::clearAll();

    // If we don't have a prefix, we just did a 'clearAll'
    // Remove the file as well.
    if (!parent) {
        std::string fName = systemPath(this->configDir + "/" + configName + ".conf");
        if (file_exists(fName))
            remove( fName.c_str() );
    }

    return ans;
    CRASH_REPORT_END;
}

/**
 * Override the set function so we can keep track of the changes
 * done in the buffer.
 */
ParameterMap& LocalConfig::set ( const std::string& name, std::string value ) {
    CRASH_REPORT_BEGIN;

    // Update time modified
    timeModified = getTimeInMs();

    // Update key value
    ParameterMap& ans = ParameterMap::set(name, value);

    // Find and erase key from 'deleted'
    std::list<std::string>::iterator iItem = std::find(keysDeleted.begin(), keysDeleted.end(), prefix+name);
    if (iItem != keysDeleted.end())
        keysDeleted.erase(iItem);

    return ans;
    CRASH_REPORT_END;
}

/**
 * Override the commitChanges function from ParameterMap so we can
 * synchronize the changes with the disk.
 */
void LocalConfig::commitChanges ( ) {
    CRASH_REPORT_BEGIN;

    CVMWA_LOG("LOG", "commitChanges");

    // Synchronize changes with the disk
    this->save();

    CRASH_REPORT_END;
}

/**
 * Save parameter map to the disk, replacing any previous contents
 */
bool LocalConfig::save ( ) {
    CRASH_REPORT_BEGIN;

    // Save map to file
    bool ans = this->saveMap( configName, parameters.get() );
    if (ans) {

        // Update the time it was loaded (since the moment
        // we wrote something we have replaced it's contents)
        timeLoaded = getTimeInMs();

        // Reset 'keysDeleted'
        keysDeleted.clear();

    }

    // Return staus
    return ans;

    CRASH_REPORT_END;
}

/**
 * Load parameter map from file, replacing any local changes
 */
bool LocalConfig::load ( ) {
    CRASH_REPORT_BEGIN;

    // Load map from file
    bool ans = this->loadMap( configName, parameters.get() );
    if (ans) {

        // Update the time it was loaded
        timeLoaded = getTimeInMs();

        // Reset 'keysDeleted'
        keysDeleted.clear();

    }

    // Return staus
    return ans;

    CRASH_REPORT_END;
}

/**
 * Synchronize map contents with the file contents
 * Conflicts are resolved using 'our' changes as favoured.
 */
bool LocalConfig::sync ( ) {
    
    // If the file is missing, save it 
    std::string fName = systemPath(this->configDir + "/" + configName + ".conf");
    if (!file_exists( fName ))
        return this->save();

    // Load the time the file was modified
    unsigned long long fileModified = getFileTimeMs( fName );

    // Check for missing modifications
    if (timeModified <= timeLoaded) {
        if (fileModified > timeLoaded) {

            // [1] Memory : No changes
            //       Disk : Changed
            //         DO : Load from disk
            return this->load();

        } else {

            // [2] Memory : No changes
            //       Disk : No changes
            //         DO : We are synced
            return true;

        }
    }

    // [3] Memory : Changed
    //       Disk : No changes
    //         DO : Replace disk contents
    if (fileModified <= timeLoaded) {
        return this->save();
    }

    // [4] Memory : Changed
    //       Disk : Changed
    //         DO : Do DIFF, preferring 'ours'

    // Load file map in a new dictionary
    std::map<std::string, std::string> map;
    if (!this->loadMap( configName, &map ))
        return false;

    // Erase keys from file from which the erase() function was called
    for (std::list<std::string>::iterator it = keysDeleted.begin(); it != keysDeleted.end(); ++it) {
        std::map<std::string, std::string>::iterator jt = map.find(*it);
        if (jt != map.end()) map.erase(jt);
    }

    // Reset 'keysDeleted'
    keysDeleted.clear();

    // Update the parameters that still exist in the config file and add new ones if they are missing.
    for (std::map<std::string, std::string>::iterator it = parameters->begin(); it != parameters->end(); ++it) {
        std::string key = (*it).first;
        std::string value = (*it).second;

        // Update field
        map[key] = value;
        
    }

    // Import new keys found in the config file
    for (std::map<std::string, std::string>::iterator it = map.begin(); it != map.end(); ++it) {
        std::string key = (*it).first;
        std::string value = (*it).second;

        // Update missing
        if (parameters->find(key) == parameters->end())
            (*parameters)[key] = value;

    }

    // Save file contents
    this->saveMap( configName, &map );

    return true;
}
