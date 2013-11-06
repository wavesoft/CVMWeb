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

#ifndef CONFIG_H_4GP6DIPT
#define CONFIG_H_4GP6DIPT

#include "Utilities.h"
#include "CrashReport.h"

class LocalConfig
{
public:
    LocalConfig ();
    
    bool                        loadLines       ( std::string file, std::vector<std::string> * lines );
    bool                        loadBuffer      ( std::string file, std::string * buffer );
    bool                        loadMap         ( std::string file, std::map<std::string, std::string> * map );
    bool                        saveLines       ( std::string file, std::vector<std::string> * lines );
    bool                        saveBuffer      ( std::string file, std::string * buffer );
    bool                        saveMap         ( std::string file, std::map<std::string, std::string> * map );
    
    std::string                 get             ( std::string name );
    std::string                 getDef          ( std::string name, std::string defaultValue );
    void                        set             ( std::string name, std::string value );

    template<typename T> T      getNum          ( std::string name );
    template<typename T> T      getNumDef       ( std::string name, T defaultValue );
    template<typename T> void   setNum          ( std::string name, T value );
    
    time_t                      getLastModified ( std::string configFile );
    bool                        exists          ( std::string configFile );
    std::string                 getPath         ( std::string configFile );

private:
    std::string                         configDir;
    std::map<std::string,std::string>   globalConfig;
    
};


#endif /* end of include guard: CONFIG_H_4GP6DIPT */
