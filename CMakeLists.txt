cmake_minimum_required(VERSION 3.0)

# to bypass -rdynamic linker error on CMake test, you need to declare this _before_ project declaration
set( CMAKE_C_COMPILER_WORKS 1 )
set( CMAKE_CXX_COMPILER_WORKS 1 )
project (SerialChronometer)

#include folder
find_package(LibXml2 REQUIRED)
include_directories(include)
include_directories(libserialport/include)

# source files
#libcsoap includes libsoap and libnanohttp. libsoap is not used
file(GLOB SOURCES "src/*.c")
file(GLOB INCLUDES "include/*.h")
include_directories( libserialport/include/ libcsoap-1.1.0/include/ ${LIBXML2_INCLUDE_DIR})

# module libraries definition for "generic" module
add_library(generic SHARED src/modules/generic.c src/debug.c)
target_compile_options(generic PRIVATE -DADD_EXPORTS)
set_target_properties(generic PROPERTIES PREFIX "")

# module libraries definition for OitoInnova's "digican" module
add_library(digican SHARED src/modules/digican.c src/debug.c)
target_compile_options(digican PRIVATE -DADD_EXPORTS)
set_target_properties(digican PROPERTIES PREFIX "")

# module libraries definition for Galican's canometer web interface module
add_library(canometroweb SHARED src/modules/canometroweb.c src/debug.c)
target_compile_options(canometroweb PRIVATE -DADD_EXPORTS)
set_target_properties(canometroweb PROPERTIES PREFIX "")

#executable declaration
add_executable(SerialChronometer ${SOURCES} ${INCLUDES})
# target_compile_options(SerialChronometer PRIVATE -static)
# target_link_options(SerialChronometer PRIVATE -static)

# add libcsoap library
if (${CMAKE_MAKE_PROGRAM} STREQUAL "/usr/bin/mingw32-make")
    set(dlfcn-win32_DIR /usr/i686-w64-mingw32/sys-root/mingw/share/dlfcn-win32)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_CROSSCOMPILING 1)
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static")
    # target_link_options(SerialChronometer PRIVATE -static)
    target_compile_options(SerialChronometer PRIVATE -static -DCURL_STATICLIB)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/lib/mingw32/libserialport.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/mingw32/libnanohttp.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/mingw32/libcsoap.a
            dlfcn-win32::dl
            curl
            idn2
            ssh2
            ssl
            crypto
            gdi32
            z
            wldap32
            setupapi
            ws2_32
            pthread
    )
    target_link_libraries(
            generic
            ${CMAKE_SOURCE_DIR}/libserialport/lib/mingw32/libserialport.a
            dlfcn-win32::dl
            setupapi
    )
    target_link_libraries(
            digican
            ${CMAKE_SOURCE_DIR}/libserialport/lib/mingw32/libserialport.a
            dlfcn-win32::dl
            setupapi
    )
    target_link_libraries(
            canometroweb
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/mingw32/libcsoap.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/mingw32/libnanohttp.a
            dlfcn-win32::dl
            xml2
            ws2_32
            setupapi
    )
else()
    find_package (Threads REQUIRED)
    target_link_libraries(
            SerialChronometer
            ${CMAKE_SOURCE_DIR}/libserialport/lib/linux/libserialport.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/linux/libnanohttp.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/linux/libcsoap.a
            pthread
            crypto
            curl
            dl
    )
    target_link_libraries(
            # libnanohttp and csoap needs to be compiled with -fpic option to be included in DLL's
            canometroweb
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/linux/libnanohttp.a
            ${CMAKE_SOURCE_DIR}/libcsoap-1.1.0/lib/linux/libcsoap.a
    )
endif()