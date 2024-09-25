cmake_minimum_required(VERSION 3.16)

# Set the project name
project(SplayTreeProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the ./include directory
include_directories(include)
add_compile_options(-Wall -Wextra) 
# Add the source files
set(SOURCES_TREE
    src/sync/lock.cc
    src/sync/epoch.cc
    test/tree.cc
)

set(SOURCES_LOCK
    src/sync/lock.cc
    test/lock.cc)

set(SOURCES_TEST
    src/sync/lock.cc
    src/sync/epoch.cc
    src/sync/guard.cc 
    src/common/utils.cc
    test/test.cc)

# Add the executable
add_executable(outputTree ${SOURCES_TREE})

add_executable(outputLock ${SOURCES_LOCK})

add_executable(outputTest ${SOURCES_TEST})
