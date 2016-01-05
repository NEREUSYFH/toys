
cmake_minimum_required (VERSION 2.8.11)

project (fiber)

set (CMAKE_VERBOSE_MAKEFILE off)

set (CMAKE_FILES_DIRECTORY ${CMAKE_BINARY_DIR})

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output) 

set (PROJECT_SOURCE_DIR ${CMAKE_BINARY_DIR}/..)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -g -O0 -DDEBUG")

include_directories (${PROJECT_SOURCE_DIR}/include)

set (TEST_SRC_DIR ${PROJECT_SOURCE_DIR}/test)

add_executable (test_server ${TEST_SRC_DIR}/test_server.cpp )
add_executable (test_client ${TEST_SRC_DIR}/test_client.cpp)



