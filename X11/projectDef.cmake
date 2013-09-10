#/**********************************************************\ 
# Auto-generated X11 project definition file for the
# CernVM Web API project
#\**********************************************************/

# X11 template platform definition CMake file
# Included from ../CMakeLists.txt

# remember that the current source dir is the project root; this file is in X11/
file (GLOB PLATFORM RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    X11/[^.]*.cpp
    X11/[^.]*.h
    X11/[^.]*.cmake
    )

SOURCE_GROUP(X11 FILES ${PLATFORM})

# Add option for browser confirm
option(BROWSER_CONFIRM "Use the insecure browser alert() function instead of the platform's native API")
if (BROWSER_CONFIRM)
	add_definitions(-DBROWSER_CONFIRM)
endif(BROWSER_CONFIRM)

# use this to add preprocessor definitions
add_definitions(
)

# Add definitions depending on build type
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  ADD_DEFINITIONS(-DDEBUG)   
ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")

set (SOURCES
    ${SOURCES}
    ${PLATFORM}
    )

add_x11_plugin(${PROJECT_NAME} SOURCES)

# add library dependencies here; leave ${PLUGIN_INTERNAL_DEPS} there unless you know what you're doing!
target_link_libraries(${PROJECT_NAME}
    ${PLUGIN_INTERNAL_DEPS} zlibstatic rt
    )

############################################################################################################
## The following lines were removed because of switch away from platform-dependant confirmation dialog
############################################################################################################

# Include GTK
# Use the package PkgConfig to detect GTK+ headers/library files
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
#include_directories(${GTK3_INCLUDE_DIRS})
#link_directories(${GTK3_LIBRARY_DIRS})

# Add libraries
#FIND_LIBRARY(X11_LIBRARY X11)
#FIND_LIBRARY(GTHREAD_LIBRARY gthread-2.0)

# Add other flags to the compiler
#add_definitions(${GTK3_CFLAGS_OTHER})

# Target link :  ${GTK3_LIBRARIES} ${X11_LIBRARY} ${GTHREAD_LIBRARY}

# Produce debug information (for crash reporting)
add_definitions(-g -rdynamic)
