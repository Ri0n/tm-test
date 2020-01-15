cmake_minimum_required(VERSION 3.10)

project(tekmonks)
add_executable(${PROJECT_NAME} "main.cpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )

add_subdirectory(src)
target_link_libraries(${PROJECT_NAME} tmlib)

option(PACKAGE_TESTS "Build the tests" ON)
if(PACKAGE_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()
