cmake_minimum_required(VERSION 3.16)
project(RS_ASIO CXX)

# Cross-compile from Linux with mingw-w64:
#   cmake -S . -B build/x86 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-i686.cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/x86
# Output: build/x86/avrt.dll and build/x86/RS_ASIO.dll
# Only 32-bit is supported: Rocksmith 2014 is a 32-bit game and the patcher/thunks
# are x86-specific.

if(NOT WIN32)
    message(FATAL_ERROR "Target is Windows; pass a mingw-w64 toolchain file (see cmake/).")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Only 32-bit builds are supported (use cmake/mingw-w64-i686.cmake).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# DLLs are loaded by the game: don't depend on mingw runtime DLLs being present.
add_compile_definitions(_WINDOWS _USRDLL UNICODE _UNICODE)
add_link_options(-static -static-libgcc -static-libstdc++)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options(-s)
endif()

# --- avrt.dll: forwarding proxy that loads RS_ASIO.dll ---
add_library(avrt SHARED
    avrt/avrt.cpp
    avrt/dllmain.cpp
    avrt/stdafx.cpp
    avrt/avrt.def)
target_compile_definitions(avrt PRIVATE AVRT_EXPORTS)
set_target_properties(avrt PROPERTIES PREFIX "")

# --- RS_ASIO.dll ---
file(GLOB RSASIO_SOURCES CONFIGURE_DEPENDS RS_ASIO/*.cpp)
add_library(RS_ASIO SHARED ${RSASIO_SOURCES} RS_ASIO/exports.def)
target_compile_definitions(RS_ASIO PRIVATE RSASIO_EXPORTS _CRT_SECURE_NO_WARNINGS)
target_link_libraries(RS_ASIO PRIVATE ole32 uuid ksguid propsys psapi)
set_target_properties(RS_ASIO PROPERTIES PREFIX "")
