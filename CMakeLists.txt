cmake_minimum_required(VERSION 3.21)

project(CRIPTO
    VERSION 1.0.0
    DESCRIPTION "Trabajo Práctico de Implementación - Criptografía y Seguridad"
    LANGUAGES C
)

# Standard C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Default to Debug build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type (Debug or Release)" FORCE)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Address & Undefined Behavior Sanitizers (default ON, matching Makefile)
option(ENABLE_SANITIZERS "Enable Address and Undefined sanitizers" ON)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# Project core library (all source files in src/ except main.c)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(stego_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(stego_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Main executable
add_executable(stegobmp "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
target_link_libraries(stegobmp PRIVATE stego_core)

# Tests
option(BUILD_TESTING "Build test suite" ON)
if(BUILD_TESTING)
    enable_testing()

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/test/test_*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*_test.c"
    )
    file(GLOB TEST_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.h")

    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SRC} ${TEST_HEADERS})
        target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test
        )
        target_link_libraries(${TEST_NAME} PRIVATE stego_core)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()
