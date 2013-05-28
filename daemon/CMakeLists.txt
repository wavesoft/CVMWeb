cmake_minimum_required (VERSION 2.6)
project ( CVMWADaemon )
include_directories( .. )

# Find FireBreath libs dir
set( FB_LIB_DIR "${PROJECT_SOURCE_DIR}/../../../src/libs" )
set( FB_BOOST_DIR "${PROJECT_SOURCE_DIR}/../../../src/3rdParty/boost" )

# Check if we can use libCURL
FIND_PACKAGE(CURL)
IF(CURL_FOUND)
	include_directories( ${CURL_INCLUDE_DIRS} )
else()
	if (EXISTS ${FB_LIB_DIR}/curl/curl/lib/libcurl.lib )
		set( CURL_INCLUDE_DIRS ${FB_LIB_DIR}/curl/curl/include )
		set( CURL_LIBRARIES ${FB_LIB_DIR}/curl/curl/lib/libcurl.lib )
		include_directories( ${CURL_INCLUDE_DIRS} )
		MESSAGE("Using cURL shipped with FireBreath")
	else()
		MESSAGE(SEND_ERROR "Could not find cURL on your system")
	endif()
	add_definitions (-DCURL_STATICLIB)
ENDIF(CURL_FOUND)

# Check if we can use OpenSSL
FIND_PACKAGE(OpenSSL)
IF (OPENSSL_FOUND)
	include_directories( ${OPENSSL_INCLUDE_DIRS} )
else()
	if (EXISTS ${FB_LIB_DIR}/openssl/openssl/lib/libeay32.lib )
		set( OPENSSL_INCLUDE_DIRS ${FB_LIB_DIR}/openssl/openssl/include )
		set( OPENSSL_LIBRARIES 
			${FB_LIB_DIR}/openssl/openssl/lib/libeay32.lib 
			${FB_LIB_DIR}/openssl/openssl/lib/ssleay32.lib 
			)
		include_directories( ${OPENSSL_INCLUDE_DIRS} )
		MESSAGE("Using OpenSSL shipped with FireBreath")
	else()
		MESSAGE(SEND_ERROR "Could not find OpenSSL on your system")
	endif()
ENDIF(OPENSSL_FOUND)

# Use FireBreath boost
FIND_PACKAGE( Boost 1.40 COMPONENTS thread REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

# Locate the platform-specific source files
if (WIN32)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Win/*.cpp  )
	include ( ${PROJECT_SOURCE_DIR}/Win/platform.cmake )
elseif(APPLE)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Mac/*.mm  )
	include ( ${PROJECT_SOURCE_DIR}/Mac/platform.cmake )
elseif(UNIX)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Linux/*.cpp  )
	include ( ${PROJECT_SOURCE_DIR}/Linux/platform.cmake )
ENDIF(WIN32)

# Sources
add_executable( ${PROJECT_NAME} 
	${PROJECT_SOURCE_DIR}/daemon.cpp 
	${PLATFORM_SOURCES}
	${PROJECT_SOURCE_DIR}/../DaemonCtl.cpp
	${PROJECT_SOURCE_DIR}/../Hypervisor.cpp
	${PROJECT_SOURCE_DIR}/../Virtualbox.cpp
	${PROJECT_SOURCE_DIR}/../ThinIPC.cpp
	${PROJECT_SOURCE_DIR}/../contextiso.cpp
	${PROJECT_SOURCE_DIR}/../Utilities.cpp
	)

# Platform-specific link details
if (WIN32)
	include ( ${PROJECT_SOURCE_DIR}/Win/link.cmake )
elseif(APPLE)
	include ( ${PROJECT_SOURCE_DIR}/Mac/link.cmake )
elseif(UNIX)
	include ( ${PROJECT_SOURCE_DIR}/Linux/link.cmake )
ENDIF(WIN32)

# Libraries
target_link_libraries (${PROJECT_NAME} ${CURL_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${OPENSSL_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${Boost_LIBRARIES} )
