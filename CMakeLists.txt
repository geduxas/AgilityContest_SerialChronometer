cmake_minimum_required(VERSION 3.0)

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

# module libraries definition
add_library(generic SHARED src/modules/generic.c src/debug.c)
target_compile_options(generic PRIVATE -DADD_EXPORTS)
set_target_properties(generic PROPERTIES PREFIX "")

#executable declaration
add_executable(SerialChronometer ${SOURCES} ${INCLUDES})
# target_compile_options(SerialChronometer PRIVATE -static)
# target_link_options(SerialChronometer PRIVATE -static)

# add libcsoap library
if (${CMAKE_MAKE_PROGRAM} STREQUAL "/usr/bin/mingw32-make")
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_CROSSCOMPILING 1)
    target_link_options(SerialChronometer PRIVATE -static)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/mingw32/libserialport.a
            dlfcn-win32::dl
            setupapi
            ws2_32
            pthread
    )
    target_link_libraries(
            generic
            ${CMAKE_SOURCE_DIR}/libserialport/mingw32/libserialport.a
            dlfcn-win32::dl
            setupapi
    )
else()
    find_package (Threads REQUIRED)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/linux/libserialport.a
            pthread
            dl
    )
    target_link_libraries(
            generic
            ${CMAKE_SOURCE_DIR}/libserialport/mingw32/libserialport.a
    )
endif()