# MIT License
#
# Copyright (c) 2015-2024 Vector 35 Inc
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.24)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
project(BinjaLua C CXX)

set(CMAKE_CXX_STANDARD 17)

# Fetch dependencies
include(FetchContent)

# Fetch Lua 5.4 from source
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.8.tar.gz
    URL_HASH SHA256=4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
)

FetchContent_MakeAvailable(lua)

# Fetch sol2 for modern C++ Lua bindings
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.3.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sol2)

# Build Lua as a static library
set(LUA_SOURCE_DIR ${lua_SOURCE_DIR}/src)

set(LUA_CORE_SOURCES
    ${LUA_SOURCE_DIR}/lapi.c
    ${LUA_SOURCE_DIR}/lcode.c
    ${LUA_SOURCE_DIR}/lctype.c
    ${LUA_SOURCE_DIR}/ldebug.c
    ${LUA_SOURCE_DIR}/ldo.c
    ${LUA_SOURCE_DIR}/ldump.c
    ${LUA_SOURCE_DIR}/lfunc.c
    ${LUA_SOURCE_DIR}/lgc.c
    ${LUA_SOURCE_DIR}/llex.c
    ${LUA_SOURCE_DIR}/lmem.c
    ${LUA_SOURCE_DIR}/lobject.c
    ${LUA_SOURCE_DIR}/lopcodes.c
    ${LUA_SOURCE_DIR}/lparser.c
    ${LUA_SOURCE_DIR}/lstate.c
    ${LUA_SOURCE_DIR}/lstring.c
    ${LUA_SOURCE_DIR}/ltable.c
    ${LUA_SOURCE_DIR}/ltm.c
    ${LUA_SOURCE_DIR}/lundump.c
    ${LUA_SOURCE_DIR}/lvm.c
    ${LUA_SOURCE_DIR}/lzio.c
)

set(LUA_LIB_SOURCES
    ${LUA_SOURCE_DIR}/lauxlib.c
    ${LUA_SOURCE_DIR}/lbaselib.c
    ${LUA_SOURCE_DIR}/lcorolib.c
    ${LUA_SOURCE_DIR}/ldblib.c
    ${LUA_SOURCE_DIR}/liolib.c
    ${LUA_SOURCE_DIR}/lmathlib.c
    ${LUA_SOURCE_DIR}/loadlib.c
    ${LUA_SOURCE_DIR}/loslib.c
    ${LUA_SOURCE_DIR}/lstrlib.c
    ${LUA_SOURCE_DIR}/ltablib.c
    ${LUA_SOURCE_DIR}/lutf8lib.c
    ${LUA_SOURCE_DIR}/linit.c
)

add_library(lua_static STATIC ${LUA_CORE_SOURCES} ${LUA_LIB_SOURCES})

target_include_directories(lua_static PUBLIC ${LUA_SOURCE_DIR})

# Platform-specific settings for Lua
if(UNIX)
    target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
    if(NOT APPLE)
        target_compile_definitions(lua_static PRIVATE LUA_USE_DLOPEN)
        target_link_libraries(lua_static PUBLIC dl)
    endif()
elseif(WIN32)
    target_compile_definitions(lua_static PRIVATE LUA_USE_WINDOWS)
endif()

# Set up Lua variables for the main project
set(LUA_INCLUDE_DIRS ${LUA_SOURCE_DIR})
set(LUA_LIBRARIES lua_static)

set(HEADLESS 1)
find_path(
        BN_API_PATH
        NAMES binaryninjaapi.h
        # List of paths to search for the clone of the api
        HINTS ../.. binaryninja-api $ENV{BN_API_PATH}
        REQUIRED
)
add_subdirectory(${BN_API_PATH} api)

# Binding sources (sol2-based)
set(BINDING_SOURCES
    bindings/common.cpp
    bindings/basicblock.cpp
    bindings/binaryview.cpp
    bindings/function.cpp
    bindings/instruction.cpp
    bindings/variable.cpp
    bindings/datavariable.cpp
    bindings/types.cpp
    bindings/tag.cpp
    bindings/il.cpp
    bindings/flowgraph.cpp
)

add_library(${PROJECT_NAME} SHARED
    library.cpp
    luascriptingprovider.cpp
    ${BINDING_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC binaryninjaapi ${LUA_LIBRARIES} sol2::sol2)

bn_install_plugin(${PROJECT_NAME})
