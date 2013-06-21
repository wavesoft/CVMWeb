cmake_minimum_required (VERSION 2.6)
project ( test )
include_directories( .. )

# Find FireBreath libs dir
set( FB_LIB_DIR "${PROJECT_SOURCE_DIR}/../../../src/libs" )
set( FB_BOOST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../../src/3rdParty/boost" )
set( FB_BOOST_LIB_DIR "${PROJECT_SOURCE_DIR}/../../../build/boost/libs" )

# External libraries
set( EXTERN_ZLIB "../extern/zlib-1.2.8" )

# Static build
SET(BUILD_SHARED_LIBS OFF)

# Add definitions depending on build type
if (WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(/D "DEBUG")   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(/D "NDEBUG")
	ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
else(WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(-DDEBUG)   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(-DNDEBUG)
	ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
endif(WIN32)

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

# Use FireBreath (already built) boost libraries
ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)
INCLUDE_DIRECTORIES( ${FB_BOOST_INCLUDE_DIR} )
if (WIN32)
	file ( GLOB BOOST_LIBRARIES 
		${FB_BOOST_LIB_DIR}/thread/${CMAKE_BUILD_TYPE}/*
		${FB_BOOST_LIB_DIR}/system/${CMAKE_BUILD_TYPE}/* )
elseif(APPLE)
	file ( GLOB BOOST_LIBRARIES 
		${FB_BOOST_LIB_DIR}/thread/${CMAKE_BUILD_TYPE}/*
		${FB_BOOST_LIB_DIR}/system/${CMAKE_BUILD_TYPE}/* )
elseif(UNIX)
	file ( GLOB BOOST_LIBRARIES 
		${FB_BOOST_LIB_DIR}/thread/*.a
		${FB_BOOST_LIB_DIR}/system/*.a )
ENDIF(WIN32)
IF("${FB_BOOST_LIB_DIR}" STREQUAL "")
	MESSAGE( FATAL_ERROR "No BOOST libraries were found for ${CMAKE_BUILD_TYPE} configuration! Please build the plugin first..." )
ENDIF()

# Build zlib statically from the resources
add_subdirectory( ${EXTERN_ZLIB} ${CMAKE_BINARY_DIR}/extern/zlib )
set( LIBZ_LIBRARIES zlibstatic )

# Sources
add_executable( ${PROJECT_NAME} 
	${PROJECT_SOURCE_DIR}/test.cpp 
	${PLATFORM_SOURCES}
	${PROJECT_SOURCE_DIR}/../DaemonCtl.cpp
	${PROJECT_SOURCE_DIR}/../Hypervisor.cpp
	${PROJECT_SOURCE_DIR}/../Virtualbox.cpp
	${PROJECT_SOURCE_DIR}/../ThinIPC.cpp
	${PROJECT_SOURCE_DIR}/../contextiso.cpp
	${PROJECT_SOURCE_DIR}/../Utilities.cpp
	${PROJECT_SOURCE_DIR}/../LocalConfig.cpp
	${PROJECT_SOURCE_DIR}/../gzstream.cpp
	${PROJECT_SOURCE_DIR}/../floppyIO.cpp
	${PROJECT_SOURCE_DIR}/../DownloadProvider.cpp
	)

# Libraries
target_link_libraries ( ${PROJECT_NAME} ${CURL_LIBRARIES} )
target_link_libraries ( ${PROJECT_NAME} ${OPENSSL_LIBRARIES} )
target_link_libraries ( ${PROJECT_NAME} ${BOOST_LIBRARIES} )
target_link_libraries ( ${PROJECT_NAME} ${LIBZ_LIBRARIES} )
