cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ChatApp LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/ca/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_executable(server ${SOURCES})

target_include_directories(server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wshadow
    -Wunused
    -Wno-error=unused-parameter
    -Wswitch-enum
    -pedantic-errors
)
