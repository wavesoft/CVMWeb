cmake_minimum_required (VERSION 2.6)
project ( mongoose )

# Includes for mongoose
include_directories( ${PROJECT_SOURCE_DIR} )

# Sources
set( MONGOOSE_SRC ${PROJECT_SOURCE_DIR}/mongoose.c )

# Link library
add_library( mongoose STATIC ${MONGOOSE_SRC} )
