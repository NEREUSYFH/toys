
cmake_minimum_required (VERSION 2.8.11)

project (fiber)


set (EXECUTABLE_OUTPUT_PATH output) 

set (CMAKE_VERBOSE_MAKEFILE on)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -Wall -Wextra -Werror -g")

include_directories (${PROJECT_SOURCE_DIR}/include)

set (TEST_SRC_DIR ${PROJECT_SOURCE_DIR}/test)

add_executable (server ${TEST_SRC_DIR}/test_server.cpp )
add_executable (client ${TEST_SRC_DIR}/test_client.cpp)



