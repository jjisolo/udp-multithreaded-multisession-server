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

set(LIB_UDP_SOURCES
    base/LibUDP/networking.cxx
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

# Include headers
include_directories(${Boost_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/base/)

# Add server and client executables
add_library   (ozzy_base       ${BASE_SOURCES}   )
add_library   (ozzy_filesystem ${LIB_FS_SOURCES} )
add_library   (ozzy_logging    ${LIB_LOG_SOURCES})
add_library   (ozzy_udp        ${LIB_UDP_SOURCES})
add_executable(ozzy_server     ${SERVER_SOURCES} )
add_executable(ozzy_client     ${CLIENT_SOURCES} )

# Macros
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
     message(STATUS "Host architecture is BIG ENDIAN")
     add_definitions(-DTARGET_DEVICE_LITTLE_ENDIAN=0)
else()
     message(STATUS "Host architecture is LITTLE ENDIAN")
     add_definitions(-DTARGET_DEVICE_LITTLE_ENDIAN=1)
endif()

if (DEFINED CLIENTS_THREAD_POOL_CAPACITY)
    add_definitions(-DCLIENTS_THREAD_POOL_CAPACITY=${CLIENTS_THREAD_POOL_CAPACITY})
    message(STATUS "Using thread pool size of " ${CLIENTS_THREAD_POOL_CAPACITY})
endif()

if (DEFINED OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES)
    add_definitions(-DOZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES=${OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES})
    message(STATUS "Using chunk memory arena size of " ${OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES})
endif()

if (DEFINED OZZY_USE_LARGE_CHUNK_MEMORY_ARENAS)
    add_definitions(-DOZZY_USE_LARGE_CHUNK_MEMORY_ARENAS=1)
    message(STATUS "Using large memory arenas(can take a lot of memory!)")
endif()


# Link libraries
target_link_libraries (ozzy_server ${Boost_LIBRARIES} ozzy_base ozzy_filesystem ozzy_logging ozzy_udp)
target_link_libraries (ozzy_client ${Boost_LIBRARIES} ozzy_base ozzy_filesystem ozzy_logging ozzy_udp)