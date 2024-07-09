cmake_minimum_required(VERSION 3.10)
project(ASIODriverRefresher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ASIODriverRefresher
        AsioDriverRefresher/main.cpp
        AsioDriverRefresher/asioHelpers.cpp
        AsioDriverRefresher/asioSharedHost.cpp
        AsioDriverRefresher/stdafx.cpp
        AsioDriverRefresher/Utils.cpp
        AsioDriverRefresher/Log.cpp
        AsioDriverRefresher/FileHelper.cpp
        AsioDriverRefresher/Config.cpp
)

set_target_properties(ASIODriverRefresher PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


target_link_libraries(ASIODriverRefresher PRIVATE propsys.lib ksuser.lib)

if(MSVC)
    target_compile_options(ASIODriverRefresher PRIVATE /W4 /MP)


    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MT")
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
