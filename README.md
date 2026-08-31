<p align="center">
  <img src="docs/images/banner.png" alt="X-Men Destiny PC Port" />
</p>

<p align="center">
  <a href="https://www.patreon.com/cw/zerkiller">
    <img src="https://img.shields.io/badge/Patreon-Become%20a%20Patron-FF424D?style=for-the-badge&logo=patreon&logoColor=white" alt="Patreon" />
  </a>
</p>

# X-Men Destiny - Xbox 360 to PC Port (ReXGlue)

A static recompilation port of **X-Men Destiny** (Xbox 360) to native PC,
built with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

ReXGlue converts Xbox 360 PowerPC XEX executables into portable C++ that runs
natively on Windows (D3D12) and Linux (Vulkan) - no emulation, no JIT at
runtime. This project contains the port configuration, custom hooks, and build
scaffolding; the ReXGlue SDK itself lives under `thirdparty/`.

## Project layout

```
.
├── CMakeLists.txt                 # Build config
├── CMakePresets.json              # Platform build presets
├── xmd_manifest.toml              # ReXGlue project manifest
├── generated/
│   ├── rexglue.cmake              # SDK boilerplate (auto-generated, DO NOT EDIT)
│   └── default/                   # codegen output (gitignored, built on demand)
├── src/
│   ├── main.cpp                   # App entry point
│   └── xmd_app.h                  # App class - override hooks here (user-owned)
├── launcher/                      # Standalone launcher (ImGui + DX11)
│   ├── CMakeLists.txt             # Launcher build config
│   └── src/                       # Launcher source (main, config, UI)
├── installer/                     # Inno Setup installer script
│   ├── xmd_installer.iss          # Installer definition
│   └── assets/                    # Default config + icons
├── tools/                         # ISO extraction + analysis tools
│   └── extract-xiso/              # Xbox 360 ISO extractor
├── game/                          # Extracted Xbox 360 game files (gitignored)
│   └── default.xex                #   <- entrypoint XEX goes here
├── metadata/                      # Achievement icons / embedded metadata
├── thirdparty/
│   └── rexglue-sdk/               # ReXGlue SDK (cloned via setup script, gitignored)
├── docs/
│   └── rexglue_notes.md           # ReXGlue workflow & command reference
├── setup.ps1 / setup.sh           # Clone SDK + init submodules
└── .gitignore
```

## Prerequisites

- **Windows 10/11 x64** (this project targets Windows/D3D12)
- **Clang 18+** (LLVM/Clang) - already detected: Clang 22
- **CMake 3.25+**
- **Ninja** build system
- **Visual Studio 2022** (for the Windows SDK / D3D12 headers)

## Getting started

### 1. Set up the SDK

```powershell
.\setup.ps1
```

This clones the ReXGlue SDK (pinned to `v0.10.0`) into `thirdparty/rexglue-sdk`
and initializes its submodules.

### 2. Provide the game files

Extract your ripped Xbox 360 ISO into `game/`. The entrypoint executable must be
at `game/default.xex` (the path set in `xmd_manifest.toml`). Keep the
original directory layout for all other assets.

> **Do not commit anything under `game/`** - it contains copyrighted assets used
> locally for recompilation only.

### 3. Build the SDK CLI (one time)

```powershell
cmake --preset win-amd64-release -DREXSDK_DIR=thirdparty\rexglue-sdk
cmake --build out\build\win-amd64-release --target rexglue
```

Add the built `rexglue.exe` to your PATH (it lives under
`thirdparty\rexglue-sdk\out\win-amd64\Release\`).

### 4. Regenerate SDK-managed files

Once `game/default.xex` exists, regenerate the SDK-managed scaffolding so it
carries the exact version/build stamp:

```powershell
rexglue init --force --project_name xmd --project_root . --xex_path game\default.xex --game_root game
```

### 5. Configure & build the port

```powershell
cmake --preset win-amd64-relwithdebinfo -DREXSDK_DIR=thirdparty\rexglue-sdk
cmake --build out\build\win-amd64-relwithdebinfo
```

The build automatically runs `rexglue codegen` (translating the XEX to C++) the
first time and whenever inputs change. Output: `out\win-amd64\RelWithDebInfo\xmd.exe`.

### 6. Run

```powershell
.\out\win-amd64\RelWithDebInfo\xmd.exe
# Useful flags:
#   --log_level=trace     verbose logging
#   --log_file=run.log    write logs to file
```

## Customizing the port

Override virtual hooks in `src/xmd_app.h` (e.g. `OnPostSetup`,
`OnCreateDialogs`, `OnConfigurePaths`). That file is **user-owned** and
preserved across `rexglue init` / `rexglue migrate`. See
`docs/rexglue_notes.md` for the full hook list and workflow reference.

## Building a release

The release consists of three components:
1. **xmd.exe** - the game port (built from `src/` + generated code)
2. **xmd_launcher.exe** - a standalone launcher with a user-friendly settings UI
3. **Installer** - an Inno Setup .exe that asks for the game ISO, extracts it,
   and installs everything

### Prerequisites for release builds

- All build prerequisites from above
- [Inno Setup 6+](https://jrsoftware.org/isdl.php) (for the installer)

### Build everything

```powershell
.\build_release.ps1
```

This builds:
- `xmd.exe` (the game) via `cmake --preset win-amd64-release`
- `xmd_launcher.exe` (the launcher) via `launcher/CMakeLists.txt`
- `extract-xiso.exe` (ISO extraction tool bundled with the installer)
- `xmd_installer.exe` (the final installer, if Inno Setup is available)

Output: `installer_output/xmd_installer.exe`

### The launcher

The launcher (`launcher/`) is a standalone C++ Win32 + ImGui + DirectX 11 app.
It provides a user-friendly UI for configuring graphics, input, and advanced
settings, then spawns `xmd.exe` with the chosen parameters. It has zero runtime
dependencies (statically linked CRT, DirectX 11 is built into Windows 10+).

### The installer

The installer (`installer/xmd_installer.iss`) is an Inno Setup script that:
1. Asks the user for their legally obtained X-Men Destiny ISO
2. Asks for an install destination
3. Extracts the ISO using the bundled `extract-xiso` tool
4. Copies the game port binaries, launcher, and runtime DLLs
5. Creates a desktop shortcut to the launcher

Users do not need to install any additional software - the installer is a
self-contained .exe.

## License

This repository contains only port scaffolding and configuration. The ReXGlue
SDK is licensed under the BSD 3-Clause License (see `thirdparty/rexglue-sdk/`).
X-Men Destiny and all game assets are property of their respective rights
holders; nothing under `game/` is distributed here.
