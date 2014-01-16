cmake_minimum_required (VERSION 2.6)
project(microhttpd C)
set(VERSION "0.9.33")

# Static build
SET(BUILD_SHARED_LIBS OFF)

# Includes
include_directories( ${PROJECT_SOURCE_DIR}/src/include )

# Find sources
file ( GLOB MICROHTTPD_SRC ${PROJECT_SOURCE_DIR}/src/microhttpd/*.c  )

# Link library
add_library(microhttpd STATIC ${MICROHTTPD_SRC})
