
# Written to work with cmake 2.6
cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

project(jsoncpp)

# Include directories
include_directories (
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set sources
file (GLOB JSONCPP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    src/*.cpp
    src/*.inl
)

# Link library
add_library (jsoncpp STATIC ${JSONCPP_SOURCES})
