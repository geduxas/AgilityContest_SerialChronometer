cmake_minimum_required(VERSION 2.8.9)

# to bypass -rdynamic linker error on CMake test, you need to declare this _before_ project declaration
set( CMAKE_C_COMPILER_WORKS 1 )
set( CMAKE_CXX_COMPILER_WORKS 1 )
project (SerialChronometer)

#include folder
include_directories(include)
include_directories(libserialport/include)

# source files
file(GLOB SOURCES "src/*.c")
file(GLOB INCLUDES "libserialport/include/*.h include/*.h")

# exe definition
add_executable(SerialChronometer ${SOURCES} ${INCLUDES})

# add libcsoap library
if (${CMAKE_MAKE_PROGRAM} STREQUAL "/usr/bin/mingw32-make")
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_CROSSCOMPILING 1)
    # mingw does not support "-rdynamic", so force empty flags and tell cmake to search static libs
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static" )
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/mingw32/libserialport.a
            setupapi
            ws2_32
            pthread
    )
else()
    find_package (Threads)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/linux/libserialport.a
            pthread
    )
endif()