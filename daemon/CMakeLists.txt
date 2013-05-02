cmake_minimum_required (VERSION 2.6)
project ( CVMWADaemon )
include_directories( .. )

FIND_PACKAGE(CURL)
IF(CURL_FOUND)
	include_directories( ${CURL_INCLUDE_DIRS} )
else()
	MESSAGE(SEND_ERROR "Could not find cURL on your system")
ENDIF(CURL_FOUND)

FIND_PACKAGE(OpenSSL)
IF (OPENSSL_FOUND)
	include_directories( ${OPENSSL_INCLUDE_DIRS} )
else()
	MESSAGE(SEND_ERROR "Could not find OpenSSL on your system")
ENDIF(OPENSSL_FOUND)

if (WIN32)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Win/*.cpp  )
elseif(APPLE)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Mac/*.cpp  )
elseif(UNIX)
	file ( GLOB PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/Linux/*.cpp  )
ENDIF(WIN32)

add_executable( CVMWADaemon 
	${PROJECT_SOURCE_DIR}/daemon.cpp 
	${PLATFORM_SOURCES}
	${PROJECT_SOURCE_DIR}/../Hypervisor.cpp
	${PROJECT_SOURCE_DIR}/../Virtualbox.cpp
	${PROJECT_SOURCE_DIR}/../ThinIPC.cpp
	${PROJECT_SOURCE_DIR}/../contextiso.cpp
	)

target_link_libraries ( CVMWADaemon ${CURL_LIBRARIES} )
target_link_libraries ( CVMWADaemon ${OPENSSL_LIBRARIES} )
