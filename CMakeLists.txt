# xmd - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(xmd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# --- Version from root VERSION file ---
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" XMD_VERSION_STRING)
string(STRIP "${XMD_VERSION_STRING}" XMD_VERSION_STRING)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/version.h"
    @ONLY
)

# Sources
set(XMD_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(xmd WIN32 ${XMD_SOURCES})
else()
    add_executable(xmd ${XMD_SOURCES})
endif()

# Include generated version.h
target_include_directories(xmd PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src")

# rexruntime links rexui privately, so ImGui include paths don't reach the
# host target. Link rex::ui directly to get them.
if(TARGET rex::ui)
    target_link_libraries(xmd PRIVATE rex::ui)
endif()

# DbgHelp.dll is loaded dynamically at runtime for crash minidumps
# (see src/xmd_app.h); no explicit link needed.

rexglue_setup_target(xmd GPU_PLUGINS xenos)
