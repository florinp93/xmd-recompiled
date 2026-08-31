cmake_minimum_required(VERSION 3.25)
project(xmd_launcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ImGui via FetchContent ---
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ImGui sources
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# --- Read version from root VERSION file ---
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" XMD_VERSION_STRING)
string(STRIP "${XMD_VERSION_STRING}" XMD_VERSION_STRING)

if(XMD_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$")
    set(XMD_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(XMD_VERSION_MINOR ${CMAKE_MATCH_2})
    set(XMD_VERSION_PATCH ${CMAKE_MATCH_3})
    set(XMD_VERSION_SUFFIX ${CMAKE_MATCH_4})
    if(XMD_VERSION_SUFFIX MATCHES "([0-9]+)")
        set(XMD_VERSION_BUILD ${CMAKE_MATCH_1})
    else()
        set(XMD_VERSION_BUILD 0)
    endif()
else()
    set(XMD_VERSION_MAJOR 0)
    set(XMD_VERSION_MINOR 1)
    set(XMD_VERSION_PATCH 0)
    set(XMD_VERSION_BUILD 0)
    set(XMD_VERSION_STRING "0.1.0")
endif()

# --- Generate resources in build dir so rc.exe finds xmd_icon.ico ---
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/resources.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated_resources.rc
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/xmd_icon.ico
    ${CMAKE_CURRENT_BINARY_DIR}/xmd_icon.ico
    COPYONLY
)

set(LAUNCHER_SOURCES
    src/main.cpp
    src/config.cpp
    src/launcher_ui.cpp
    src/updater.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated_resources.rc
)

add_executable(xmd_launcher WIN32 ${LAUNCHER_SOURCES} ${IMGUI_SOURCES})

target_include_directories(xmd_launcher PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Static link CRT so the launcher has zero runtime deps
if(MSVC)
    set_property(TARGET xmd_launcher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Link DirectX 11 (system lib, always available on Windows 10+)
target_link_libraries(xmd_launcher PRIVATE
    d3d11
    dxgi
    d3dcompiler
    dwmapi
)

# Use Windows 7+ API
if(WIN32)
    target_compile_definitions(xmd_launcher PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    )
endif()
