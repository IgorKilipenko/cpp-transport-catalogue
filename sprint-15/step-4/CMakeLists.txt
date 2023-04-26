cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME "cpp-sprint-15-step-4-less-3")
project(${PROJECT_NAME} CXX)

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/ImgConverter")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/phone-book-serialize")