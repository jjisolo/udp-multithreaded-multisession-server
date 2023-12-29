cmake_minimum_required(VERSION 3.0)
project(ozz-test-task)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost components
find_package(Boost REQUIRED COMPONENTS system program_options)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found. Please check Boost_INCLUDE_DIR and Boost_LIBRARY_DIR.")
endif()

# Source files
set(BASE_SOURCES
    base/protocol.cpp
    base/LibTT/configurable.cxx
)

set(LIB_FS_SOURCES
    base/LibFS/filesystem.cxx
    base/LibFS/thread_cache_file.cxx
)

set(LIB_LOG_SOURCES
    base/LibLog/logging.cxx
)

set(SERVER_SOURCES 
    server/main.cxx
    server/udp_server_base.cxx
    server/udp_server_v2.cxx
)

set(CLIENT_SOURCES
    client/main.cxx
    client/udp_client_base.cxx
    client/udp_client_v2.cxx
)

# Include Boost headers
include_directories(${Boost_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/base/)
#include_directories(${CMAKE_SOURCE_DIR}/base/LibLog)
#include_directories(${CMAKE_SOURCE_DIR}/base/LibUDP)
#include_directories(${CMAKE_SOURCE_DIR}/base/LibTT)

# Add server and client executables
add_library   (ozzy_base       ${BASE_SOURCES}   )
add_library   (ozzy_filesystem ${LIB_FS_SOURCES} )
add_library   (ozzy_logging    ${LIB_LOG_SOURCES})
add_executable(ozzy_server     ${SERVER_SOURCES} )
add_executable(ozzy_client     ${CLIENT_SOURCES} )

# Link libraries
target_link_libraries (ozzy_server ${Boost_LIBRARIES} ozzy_base ozzy_filesystem ozzy_logging)
target_link_libraries (ozzy_client ${Boost_LIBRARIES} ozzy_base ozzy_filesystem ozzy_logging)