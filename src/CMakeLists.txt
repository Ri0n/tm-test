cmake_minimum_required(VERSION 3.10)

message(STATUS "Check if the system supports epoll")
include(CheckSymbolExists)
check_symbol_exists(epoll_create "sys/epoll.h" EPOLL_PROTOTYPE_EXISTS)

if(!EPOLL_PROTOTYPE_EXISTS)
  message(FATAL_ERROR "Compilation without epoll is not supported")
endif(!EPOLL_PROTOTYPE_EXISTS)

find_package(OpenSSL REQUIRED)

project(tmlib)
add_library(${PROJECT_NAME} STATIC
    "httpclient.cpp"
    "device.cpp"
    "reactor.cpp"
    "reactor_epoll.cpp"
    "exception.cpp"
    "log.cpp"
    "url.cpp"
    "socket.cpp"
    "securesocket.cpp"
    "briefextractor.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)
