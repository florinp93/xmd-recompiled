#pragma once

#include <string>
#include <filesystem>

struct LaunchConfig {
    int resolution_scale = 1;
    bool native_2x_msaa = false;
    bool vsync = true;
    bool fullscreen = true;
    bool clock_no_scaling = true;
    int anisotropic_override = 0;
    bool present_dither = true;
    std::string present_effect = "bilinear";
    std::string log_level = "info";
    bool debug_overlay = false;
    std::string game_data_root;
    std::string user_data_root;
    std::string log_file = "xmd_runtime.log";

    void Load(const std::filesystem::path& path);
    void Save(const std::filesystem::path& path) const;
    std::string ToCommandLineArgs() const;
};

std::string GetMonitorResolution();
std::string ScaleResolution(int scale);

constexpr int XMD_NATIVE_WIDTH = 1280;
constexpr int XMD_NATIVE_HEIGHT = 720;
