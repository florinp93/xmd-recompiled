#include "config.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>

static std::string Trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

static std::string ParseString(const std::string& val) {
    std::string s = Trim(val);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    return s;
}

static bool ParseBool(const std::string& val) {
    std::string s = Trim(val);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s == "true" || s == "1" || s == "yes";
}

static int ParseInt(const std::string& val) {
    try { return std::stoi(Trim(val)); }
    catch (...) { return 0; }
}

void LaunchConfig::Load(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        std::string val = line.substr(eq + 1);
        if (key == "resolution_scale") resolution_scale = ParseInt(val);
        else if (key == "native_2x_msaa") native_2x_msaa = ParseBool(val);
        else if (key == "vsync") vsync = ParseBool(val);
        else if (key == "fullscreen") fullscreen = ParseBool(val);
        else if (key == "clock_no_scaling") clock_no_scaling = ParseBool(val);
        else if (key == "anisotropic_override") anisotropic_override = ParseInt(val);
        else if (key == "present_dither") present_dither = ParseBool(val);
        else if (key == "present_effect") present_effect = ParseString(val);
        else if (key == "log_level") log_level = ParseString(val);
        else if (key == "log_file") log_file = ParseString(val);
    }
}

void LaunchConfig::Save(const std::filesystem::path& path) const {
    std::ofstream f(path);
    if (!f) return;
    f << "# X-Men Destiny PC Port - Configuration\n\n";
    f << "resolution_scale = " << resolution_scale << "\n";
    f << "native_2x_msaa = " << (native_2x_msaa ? "true" : "false") << "\n";
    f << "vsync = " << (vsync ? "true" : "false") << "\n";
    f << "fullscreen = " << (fullscreen ? "true" : "false") << "\n";
    f << "clock_no_scaling = " << (clock_no_scaling ? "true" : "false") << "\n";
    f << "anisotropic_override = " << anisotropic_override << "\n";
    f << "present_dither = " << (present_dither ? "true" : "false") << "\n";
    f << "present_effect = \"" << present_effect << "\"\n";
    f << "log_level = \"" << log_level << "\"\n";
    f << "log_file = \"" << log_file << "\"\n";
}

std::string LaunchConfig::ToCommandLineArgs() const {
    std::ostringstream ss;
    ss << "--gpu_plugin xenos";
    ss << " --log_level " << log_level;
    if (debug_overlay) ss << " --debug_overlay true";
    if (!game_data_root.empty()) ss << " --game_data_root \"" << game_data_root << "\"";
    if (!user_data_root.empty()) ss << " --user_data_root \"" << user_data_root << "\"";
    return ss.str();
}

std::string GetMonitorResolution() {
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
        return std::to_string(dm.dmPelsWidth) + "x" + std::to_string(dm.dmPelsHeight);
    }
    return "1920x1080";
}

std::string ScaleResolution(int scale) {
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
        int w = dm.dmPelsWidth * scale;
        int h = dm.dmPelsHeight * scale;
        return std::to_string(w) + "x" + std::to_string(h);
    }
    int w = 1920 * scale;
    int h = 1080 * scale;
    return std::to_string(w) + "x" + std::to_string(h);
}
