cmake_minimum_required(VERSION 3.11)

project(Cypher CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../ImgLib ImgLibBuildDir)

add_executable(cypher main.cpp)

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

file(COPY ../kitty-1-cypher.ppm DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(cypher ImgLib ${SYSTEM_LIBS})

target_include_directories(cypher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../ImgLib")
