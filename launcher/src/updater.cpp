#include "updater.h"
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <regex>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

static std::string WToA(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

static std::wstring AToW(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static bool HttpGet(const std::wstring& url, std::string& out) {
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    WCHAR scheme[32] = {};
    WCHAR host[256] = {};
    WCHAR path[2048] = {};
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = ARRAYSIZE(scheme);
    uc.lpszHostName = host;
    uc.dwHostNameLength = ARRAYSIZE(host);
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = ARRAYSIZE(path);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSession = WinHttpOpen(L"xmd-launcher/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD total = 0;
    do {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &available) || available == 0) break;
        DWORD old = (DWORD)out.size();
        out.resize(old + available);
        DWORD read = 0;
        WinHttpReadData(hRequest, &out[old], available, &read);
        out.resize(old + read);
        total += read;
    } while (true);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return total > 0;
}

static bool HttpDownload(const std::wstring& url, const std::wstring& out_path) {
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    WCHAR scheme[32] = {};
    WCHAR host[256] = {};
    WCHAR path[2048] = {};
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = ARRAYSIZE(scheme);
    uc.lpszHostName = host;
    uc.dwHostNameLength = ARRAYSIZE(host);
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = ARRAYSIZE(path);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSession = WinHttpOpen(L"xmd-launcher/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    std::ofstream f(out_path, std::ios::binary);
    if (!f) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    char buf[65536];
    DWORD read = 0;
    do {
        read = 0;
        if (!WinHttpReadData(hRequest, buf, sizeof(buf), &read)) break;
        if (read) f.write(buf, read);
    } while (read > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    f.close();
    return read == 0 && f.good();
}

static std::string UnescapeJsonString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[i + 1];
            if (c == 'n') out += '\n';
            else if (c == 'r') { /* skip */ }
            else if (c == 't') out += '\t';
            else if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else { out += '\\'; out += c; }
            ++i;
        } else if (s[i] == '\r') {
            // skip
        } else {
            out += s[i];
        }
    }
    return out;
}

static std::string ExtractJsonString(const std::string& json, const std::string& key, size_t start_pos = 0) {
    size_t k = json.find("\"" + key + "\"", start_pos);
    if (k == std::string::npos) return "";
    k = json.find(':', k + key.size() + 2);
    if (k == std::string::npos) return "";
    k = json.find('"', k);
    if (k == std::string::npos) return "";
    size_t e = k + 1;
    while (e < json.size()) {
        if (json[e] == '"' && json[e - 1] != '\\') break;
        ++e;
    }
    return UnescapeJsonString(json.substr(k + 1, e - k - 1));
}

static std::string ExtractAssetDownloadUrl(const std::string& json) {
    // Find the asset named xmd_installer.exe, then its browser_download_url.
    size_t pos = 0;
    while (true) {
        size_t name_pos = json.find("\"name\"", pos);
        if (name_pos == std::string::npos) break;
        std::string name = ExtractJsonString(json, "name", name_pos);
        if (name == "xmd_installer.exe") {
            return ExtractJsonString(json, "browser_download_url", name_pos);
        }
        pos = name_pos + 1;
    }
    // Fallback to first browser_download_url
    return ExtractJsonString(json, "browser_download_url", 0);
}

struct Version {
    int major = 0, minor = 0, patch = 0, pre = 0;
    bool has_pre = false;
};

static Version ParseVersion(std::string s) {
    Version v = {};
    if (!s.empty() && (s[0] == 'v' || s[0] == 'V')) s = s.substr(1);

    std::string base, pre;
    size_t dash = s.find('-');
    if (dash != std::string::npos) {
        base = s.substr(0, dash);
        pre = s.substr(dash + 1);
        v.has_pre = true;
    } else {
        base = s;
    }

    if (sscanf(base.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch) != 3) {
        v.major = v.minor = v.patch = 0;
    }

    // pre-release may be like "alpha.3" or "alpha3" or "3"
    std::regex re("([0-9]+)");
    std::smatch m;
    if (std::regex_search(pre, m, re)) {
        v.pre = std::stoi(m[1].str());
    }
    return v;
}

static bool IsNewer(const Version& cur, const Version& rem) {
    if (rem.major != cur.major) return rem.major > cur.major;
    if (rem.minor != cur.minor) return rem.minor > cur.minor;
    if (rem.patch != cur.patch) return rem.patch > cur.patch;
    if (rem.has_pre && !cur.has_pre) return false;         // 0.1.0 > 0.1.0-alpha.3
    if (!rem.has_pre && cur.has_pre) return true;          // 0.1.0 > 0.1.0-alpha.3
    if (rem.has_pre && cur.has_pre) return rem.pre > cur.pre;
    return false;
}

std::optional<UpdateInfo> CheckForUpdate(const std::string& current_version) {
    std::string json;
    if (!HttpGet(L"https://api.github.com/repos/florinp93/xmd-recompiled/releases/latest", json))
        return std::nullopt;

    std::string tag = ExtractJsonString(json, "tag_name");
    if (tag.empty()) return std::nullopt;

    Version cur = ParseVersion(current_version);
    Version rem = ParseVersion(tag);
    if (!IsNewer(cur, rem)) return std::nullopt;

    UpdateInfo info;
    info.version = tag;
    info.changelog = ExtractJsonString(json, "body");
    info.installer_url = ExtractAssetDownloadUrl(json);
    if (info.installer_url.empty()) return std::nullopt;
    return info;
}

bool DownloadInstaller(const std::string& url, const std::filesystem::path& out_path) {
    return HttpDownload(AToW(url), out_path.wstring());
}

void RunInstallerAndExit(const std::filesystem::path& installer_path, uint32_t parent_pid) {
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    std::wstring ps_path = std::wstring(temp_path) + L"xmd_update.ps1";

    std::wstring script =
        L"param([string]$Installer, [int]$ParentPid)\n"
        L"$proc = Get-Process -Id $ParentPid -ErrorAction SilentlyContinue\n"
        L"if ($proc) { $proc | Wait-Process }\n"
        L"Start-Process -FilePath $Installer -ArgumentList '/VERYSILENT' -Verb runAs -Wait\n";

    std::string script_narrow = WToA(script);
    std::ofstream f(std::filesystem::path(ps_path), std::ios::binary);
    f.write(script_narrow.c_str(), script_narrow.size());
    f.close();

    std::wstring args = L"-ExecutionPolicy Bypass -WindowStyle Hidden -File \"" + ps_path +
                        L"\" -Installer \"" + installer_path.wstring() + L"\" -ParentPid " +
                        std::to_wstring(parent_pid);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.lpVerb = L"open";
    sei.lpFile = L"powershell.exe";
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);
}
