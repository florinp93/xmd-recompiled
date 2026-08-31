#pragma once

#include <optional>
#include <string>
#include <filesystem>

struct UpdateInfo {
    std::string version;
    std::string changelog;
    std::string installer_url;
};

// Fetch the latest GitHub release. Returns nullopt if none or current is up to date.
std::optional<UpdateInfo> CheckForUpdate(const std::string& current_version);

// Download the installer to a temp path. Returns true on success.
bool DownloadInstaller(const std::string& url, const std::filesystem::path& out_path);

// Write a small updater script, run it, and return immediately.
// The script waits for this process to exit then runs the installer silently.
void RunInstallerAndExit(const std::filesystem::path& installer_path, uint32_t parent_pid);
