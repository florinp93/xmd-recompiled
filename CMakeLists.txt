# xmd - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(xmd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(XMD_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(xmd WIN32 ${XMD_SOURCES})
else()
    add_executable(xmd ${XMD_SOURCES})
endif()

# rexruntime links rexui privately, so ImGui include paths don't reach the
# host target. Link rex::ui directly to get them.
if(TARGET rex::ui)
    target_link_libraries(xmd PRIVATE rex::ui)
endif()

rexglue_setup_target(xmd GPU_PLUGINS xenos)
