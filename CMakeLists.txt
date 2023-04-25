cmake_minimum_required(VERSION 3.14)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PROJECT_NAME "cpp-sprint-15-step-4")
project(${PROJECT_NAME} CXX)

set(CFLAGS -Wall -Werror -pedantic)
set(LINK_OPTS)
set(LDLIBS tbb pthread)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/sprint-15/step-4")