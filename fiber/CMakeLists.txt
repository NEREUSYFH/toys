
cmake_minimum_required (VERSION 2.8.11)

project (fiber)

set (CMAKE_VERBOSE_MAKEFILE off)

set (CMAKE_FILES_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/output) 

set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/)

#@set (CMAKE_CXX_COMPILER /usr/local/bin/g++) 

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -g -O0 -DDEBUG")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -g -O3")

include_directories (${PROJECT_SOURCE_DIR}/include)

link_libraries (pthread)

set (EXAMPLE_SRC_DIR ${PROJECT_SOURCE_DIR}/example)

#add_executable (http_server ${EXAMPLE_SRC_DIR}/http_server.cpp)
add_executable (echo_server ${EXAMPLE_SRC_DIR}/echo_server.cpp)

set (TEST_SRC_DIR ${PROJECT_SOURCE_DIR}/test)

#add_executable (test_server ${TEST_SRC_DIR}/test_server.cpp )
add_executable (test_client ${TEST_SRC_DIR}/test_client.cpp)



