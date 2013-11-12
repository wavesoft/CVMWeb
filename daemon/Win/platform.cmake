
# Make sure the libraries will link statically
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MTd")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -MT")

# Do not use the windows.h macros min() and max()
add_definitions(-DNOMINMAX)   
