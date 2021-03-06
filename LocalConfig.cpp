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

#include "LocalConfig.h"

/**
 * Create platform configuration file
 */
LocalConfig::LocalConfig () {
    CRASH_REPORT_BEGIN;
    this->configDir = getAppDataPath() + "/config";
    this->loadMap( "general", &globalConfig );
    CRASH_REPORT_END;
}

/**
 * Save all the lines to the given list file
 */
bool LocalConfig::saveLines ( std::string name, std::vector<std::string> * lines ) {
    CRASH_REPORT_BEGIN;
    
    // Truncate file
    std::string file = this->configDir + "/" + name + ".lst";
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) return false;
    
    // Dump the contents
    for (std::vector<std::string>::iterator i = lines->begin(); i != lines->end(); ++i) {
        std::string line = *i;
        ofs << line << std::endl;
    }
    
    // Close
    ofs.close();
    return true;
    
    CRASH_REPORT_END;
}

/**
 * Save the string buffer to the given data file
 */
bool LocalConfig::saveBuffer ( std::string name, std::string * buffer ) {
    CRASH_REPORT_BEGIN;
    
    // Truncate file
    std::string file = this->configDir + "/" + name + ".dat";
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) return false;
    
    // Dump the contents
    ofs << buffer;
    
    // Close
    ofs.close();
    return true;

    CRASH_REPORT_END;
}
/**
 * Save string map to the given config file
 */
bool LocalConfig::saveMap ( std::string name, std::map<std::string, std::string> * map ) {
    CRASH_REPORT_BEGIN;
    
    // Truncate file
    std::string file = this->configDir + "/" + name + ".conf";
    CVMWA_LOG("Config", "Saving" << file );
    std::ofstream ofs ( file.c_str() , std::ofstream::out | std::ofstream::trunc);
    if (ofs.fail()) return false;
    
    // Dump the contents
    for (std::map<std::string, std::string>::iterator it=map->begin(); it!=map->end(); ++it) {
        std::string key = (*it).first;
        std::string value = (*it).second;
        CVMWA_LOG("Config", "Storing '" << key << "=" << value << "'");
        ofs << key << "=" << value << std::endl;
    }
    
    // Close
    ofs.close();
    return true;
    
    CRASH_REPORT_END;
}

/**
 * Load all the lines from the given list file
 */
bool LocalConfig::loadLines ( std::string name, std::vector<std::string> * lines ) {
    CRASH_REPORT_BEGIN;
    
    // Load configuration
    std::string file = this->configDir + "/" + name + ".lst";
    std::ifstream ifs ( file.c_str() , std::ifstream::in);
    if (ifs.fail()) return false;
    
    // Read file
    std::string line;
    lines->clear();
    while( std::getline(ifs, line) ) {
        lines->push_back(line);
    }
    
    // Close file
    ifs.close();
    return true;
    
    CRASH_REPORT_END;
}

/**
 * Read the string buffer from the given data file
 */
bool LocalConfig::loadBuffer ( std::string name, std::string * buffer ) {
    CRASH_REPORT_BEGIN;
    
    // Load configuration
    std::string file = this->configDir + "/" + name + ".dat";
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
    
    CRASH_REPORT_END;
}

/**
 * Load string map from the given config file
 */
bool LocalConfig::loadMap ( std::string name, std::map<std::string, std::string> * map ) {
    CRASH_REPORT_BEGIN;
    
    // Load configuration
    std::string file = this->configDir + "/" + name + ".conf";
    CVMWA_LOG( "Config", "LoadingMap " << file.c_str()  );
    std::ifstream ifs ( file.c_str() , std::ifstream::in);
    if (ifs.fail()) return false;
    
    // Read file
    std::string line;
    map->clear();
    CVMWA_LOG( "Config", "Going to read map"  );
    while( std::getline(ifs, line) ) {
      CVMWA_LOG( "Config", "LoadingMap : Processing line '" << line << "'"  );
      std::istringstream is_line(line);
      std::string key;
      if( std::getline(is_line, key, '=') ) {
        std::string value;
        if( std::getline(is_line, value) ) {
          CVMWA_LOG( "Config", "LoadingMap : Setting '" << key << "' = '" << value << "'"  );
          map->insert( std::pair<std::string,std::string>(key, value) );
        }
      }
    }
    
    // Close file
    ifs.close();
    return true;
    
    CRASH_REPORT_END;
}

std::string LocalConfig::getPath( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = this->configDir + "/" + configFile;
    return file;
    CRASH_REPORT_END;
}

bool LocalConfig::exists ( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = this->configDir + "/" + configFile;
    return file_exists( file );
    CRASH_REPORT_END;
}

time_t LocalConfig::getLastModified ( std::string configFile ) {
    CRASH_REPORT_BEGIN;
    std::string file = this->configDir + "/" + configFile;

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

std::string LocalConfig::get ( std::string name ) {
    CRASH_REPORT_BEGIN;
    if (globalConfig.find(name) == globalConfig.end())
        return "";
    return globalConfig[name];
    CRASH_REPORT_END;
}

std::string LocalConfig::getDef  ( std::string name, std::string defaultValue ) {
    CRASH_REPORT_BEGIN;
    if (globalConfig.find(name) == globalConfig.end())
        return defaultValue;
    return globalConfig[name];
    CRASH_REPORT_END;
}

template <typename T> T LocalConfig::getNum ( std::string name ) {
    CRASH_REPORT_BEGIN;
    if (globalConfig.find(name) == globalConfig.end())
        return 0;
    return ston<T>(globalConfig[name]);
    CRASH_REPORT_END;
}

template <typename T> T LocalConfig::getNumDef ( std::string name, T defaultValue ) {
    CRASH_REPORT_BEGIN;
    if (globalConfig.find(name) == globalConfig.end())
        return defaultValue;
    return ston<T>(globalConfig[name]);
    CRASH_REPORT_END;
}

void LocalConfig::set ( std::string name, std::string value ) {
    CRASH_REPORT_BEGIN;
    globalConfig[name] = value;
    this->saveMap( "general", &globalConfig );
    CRASH_REPORT_END;
}

template <typename T> void LocalConfig::setNum ( std::string name, T value ) {
    CRASH_REPORT_BEGIN;
    globalConfig[name] = ntos<T>( value );
    this->saveMap( "general", &globalConfig );
    CRASH_REPORT_END;
}

/* Expose template implementations */
template int LocalConfig::getNum<int>( std::string );
template long LocalConfig::getNum<long>( std::string );
template int LocalConfig::getNumDef<int>( std::string, int );
template long LocalConfig::getNumDef<long>( std::string, long );
template void LocalConfig::setNum<int>( std::string, int value );
template void LocalConfig::setNum<long>( std::string, long value );
