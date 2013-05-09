cmake_minimum_required (VERSION 2.6)
project (CVMTest)
include_directories( .. )

# Find FireBreath libs dir
set( FB_LIB_DIR "${PROJECT_SOURCE_DIR}/../../../src/libs" )

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

# Static build
if (WIN32)
	set (CMAKE_EXE_LINKER_FLAGS -static)
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MTd")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -MT")
endif()

# Sources
add_executable(CVMTest 
	${PROJECT_SOURCE_DIR}/test.cpp 
	${PROJECT_SOURCE_DIR}/../Hypervisor.cpp
	${PROJECT_SOURCE_DIR}/../Virtualbox.cpp
	${PROJECT_SOURCE_DIR}/../ThinIPC.cpp
	${PROJECT_SOURCE_DIR}/../DaemonCtl.cpp
	${PROJECT_SOURCE_DIR}/../contextiso.cpp
	)

# Windows libraries
if (WIN32)
	target_link_libraries (CVMTest ws2_32.lib)
	target_link_libraries (CVMTest wsock32.lib)
endif()

# Libraries
target_link_libraries (CVMTest ${CURL_LIBRARIES})
target_link_libraries (CVMTest ${OPENSSL_LIBRARIES})
