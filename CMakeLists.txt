cmake_minimum_required(VERSION 3.0)

set (PROJECT_NAME "sst25vfxxxx_spi")
set (PROJECT_FILES main.c)

project(${PROJECT_NAME} LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

include_directories(SYSTEM ${CMAKE_SOURCE_DIR} pytest_c_testrunner/test/)
add_executable(${PROJECT_NAME} ${PROJECT_FILES})

