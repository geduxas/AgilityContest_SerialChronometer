cmake_minimum_required(VERSION 2.8.9)
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
    # mingw does not support "-rdynamic", so use
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/mingw32/libserialport.a
            pthread
            setupapi
            wsock32
    )
else()
    find_package (Threads)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/linux/libserialport.a
            pthread
    )
endif()