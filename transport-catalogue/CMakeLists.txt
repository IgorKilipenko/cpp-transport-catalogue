cmake_minimum_required(VERSION 3.14)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PROJECT_NAME "transport_catalogue")
project(${PROJECT_NAME} CXX)

set(SRC_DIR ${PROJECT_SOURCE_DIR})
set(MAIN_FILE ${SRC_DIR}/main.cpp)
set(PROTO_DIR ${SRC_DIR})

set(LIBS 
    ${MAIN_FILE}
    ${SRC_DIR}/transport_catalogue.cpp
    ${SRC_DIR}/json_builder.cpp
    ${SRC_DIR}/svg.cpp
    ${SRC_DIR}/json.cpp
    ${SRC_DIR}/domain.cpp
    ${SRC_DIR}/geo.cpp
    ${SRC_DIR}/json_reader.cpp
    ${SRC_DIR}/map_renderer.cpp
    ${SRC_DIR}/request_handler.cpp
    ${SRC_DIR}/transport_router.cpp
    ${SRC_DIR}/serialization.cpp
)

set(PROTO_FILES 
    ${PROTO_DIR}/transport_catalogue.proto 
    ${PROTO_DIR}/svg.proto
    ${PROTO_DIR}/map_renderer.proto
)

set(CFLAGS -Wall -Werror -pedantic)
set(DEFS)
set(LINK_OPTS)
set(LDLIBS tbb pthread)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(${PROJECT_NAME} ${PROTO_SRCS} ${PROTO_HDRS} ${LIBS})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEFS})
target_link_options(${PROJECT_NAME} PRIVATE ${LINK_OPTS})
target_link_libraries(${PROJECT_NAME} ${LDLIBS})

target_include_directories(${PROJECT_NAME} PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(${PROJECT_NAME} "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)