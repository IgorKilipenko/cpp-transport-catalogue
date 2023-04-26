cmake_minimum_required(VERSION 3.14)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PROJECT_NAME "sprint15-transport-catalogue")
project(${PROJECT_NAME} CXX)

set(CMAKE_PREFIX_PATH "./third_party/protobuf/package")

add_subdirectory("./transport-catalogue")