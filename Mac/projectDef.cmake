#/**********************************************************\ 
# Auto-generated Mac project definition file for the
# CernVM Web API project
#\**********************************************************/

# Mac template platform definition CMake file
# Included from ../CMakeLists.txt

# remember that the current source dir is the project root; this file is in Mac/
file (GLOB PLATFORM RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    Mac/[^.]*.cpp
    Mac/[^.]*.h
    Mac/[^.]*.mm
    Mac/[^.]*.cmake
    )

# use this to add preprocessor definitions
add_definitions(
)

# Add definitions depending on build type
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  ADD_DEFINITIONS(-DDEBUG)   
ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")

SOURCE_GROUP(Mac FILES ${PLATFORM})

set (SOURCES
    ${SOURCES}
    ${PLATFORM}
    )

set(PLIST "Mac/bundle_template/Info.plist")
set(STRINGS "Mac/bundle_template/InfoPlist.strings")
set(LOCALIZED "Mac/bundle_template/Localized.r")

add_mac_plugin(${PROJECT_NAME} ${PLIST} ${STRINGS} ${LOCALIZED} SOURCES)

# add library dependencies here; leave ${PLUGIN_INTERNAL_DEPS} there unless you know what you're doing!
target_link_libraries(${PROJECT_NAME}
    ${PLUGIN_INTERNAL_DEPS} zlibstatic
    )

# Include QT
#find_package (Qt4)
#include(${QT_USE_FILE})
#add_definitions(${QT_DEFINITIONS})
#include_directories(${QT_INCLUDE_DIRS})
#target_link_libraries ( ${PROJECT_NAME} ${QT_LIBRARIES})

#To create a DMG, include the following file
#include(Mac/installer.cmake)

# Produce debug information (for crash reporting)
add_definitions(-g)
