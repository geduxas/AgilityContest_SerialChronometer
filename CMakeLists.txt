cmake_minimum_required(VERSION 2.8.9)
project (SerialChronometer)

#include folder
include_directories(include)
include_directories(libs/libserialport/include libs/logging libs/parseini libs/wingetopt)

# source files
file(GLOB SOURCES "src/*.c")
file(GLOB SOURCES "libs/*/*.c")
file(GLOB INCLUDES "include/*.h")
file(GLOB INCLUDES "libs/*/*.h")

# exe definition
add_executable(SerialChronometer ${SOURCES} ${INCLUDES} src/main.c)

# add libcsoap library
if (${CMAKE_MAKE_PROGRAM} STREQUAL "/usr/bin/mingw32-make")
    # mingw does not support "-rdynamic", so use
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libs/libserialport/mingw32/libserialport.a
            wsock32
    )
else()
    find_package (Threads)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libs/libserialport/linux/libserialport.a
    )
endif()