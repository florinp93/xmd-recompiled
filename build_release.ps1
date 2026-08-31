# build_release.ps1 - Build the launcher, game, and prepare the installer.
#
# Usage: .\build_release.ps1
#
# Prerequisites:
#   - ReXGlue SDK cloned at thirdparty\rexglue-sdk (run setup.ps1 first)
#   - game\default.xex present (extract your ISO first)
#   - Inno Setup 6+ installed (for the installer .exe)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdkDir = Join-Path $root "thirdparty\rexglue-sdk"

Write-Host "== X-Men Destiny - Release Build ==" -ForegroundColor Cyan

# --- Check prerequisites ---
if (-not (Test-Path (Join-Path $sdkDir ".git"))) {
    Write-Host "ERROR: ReXGlue SDK not found. Run .\setup.ps1 first." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path (Join-Path $root "game\default.xex"))) {
    Write-Host "ERROR: game\default.xex not found. Extract your ISO into game\ first." -ForegroundColor Red
    exit 1
}

# --- Build the game (xmd.exe) ---
Write-Host "`n--- Building xmd.exe ---" -ForegroundColor Yellow
cmake --preset win-amd64-release -S $root -DREXSDK_DIR=$sdkDir
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for xmd" }

cmake --build "$root\out\build\win-amd64-release" --config Release
if ($LASTEXITCODE -ne 0) { throw "xmd build failed" }

# --- Build the launcher ---
Write-Host "`n--- Building xmd_launcher.exe ---" -ForegroundColor Yellow
$launcherDir = Join-Path $root "launcher"
$launcherBuild = Join-Path $root "out\build\launcher-release"

cmake -S $launcherDir -B $launcherBuild -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for launcher" }

cmake --build $launcherBuild --config Release
if ($LASTEXITCODE -ne 0) { throw "Launcher build failed" }

# --- Copy launcher to installer staging ---
$staging = Join-Path $root "installer\build"
New-Item -ItemType Directory -Path "$staging\launcher\Release" -Force | Out-Null
New-Item -ItemType Directory -Path "$staging\xmd\Release" -Force | Out-Null

Copy-Item "$launcherBuild\xmd_launcher.exe" "$staging\launcher\Release\" -Force

# --- Copy game binaries to installer staging ---
$gameBuild = "$root\out\build\win-amd64-release"
Copy-Item "$gameBuild\xmd.exe" "$staging\xmd\Release\" -Force
Get-ChildItem "$gameBuild\*.dll" | Copy-Item -Destination "$staging\xmd\Release\" -Force

# --- Build extract-xiso ---
Write-Host "`n--- Building extract-xiso ---" -ForegroundColor Yellow
$xisoSrc = Join-Path $root "tools\extract-xiso"
$xisoBuild = Join-Path $root "out\build\extract-xiso"

if (-not (Test-Path "$xisoBuild\extract-xiso.exe")) {
    cmake -S $xisoSrc -B $xisoBuild -G Ninja -DCMAKE_C_COMPILER=clang
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for extract-xiso" }
    cmake --build $xisoBuild
    if ($LASTEXITCODE -ne 0) { throw "extract-xiso build failed" }
}

New-Item -ItemType Directory -Path "$staging\..\tools\extract-xiso\build" -Force | Out-Null
Copy-Item "$xisoBuild\extract-xiso.exe" "$root\installer\tools\extract-xiso\build\" -Force

# --- Build the installer ---
Write-Host "`n--- Building installer ---" -ForegroundColor Yellow
$iscc = Get-Command "iscc" -ErrorAction SilentlyContinue
if ($iscc) {
    & iscc "$root\installer\xmd_installer.iss"
    if ($LASTEXITCODE -ne 0) { throw "Installer build failed" }
    Write-Host "`nInstaller created at: $root\installer_output\xmd_installer.exe" -ForegroundColor Green
} else {
    Write-Host "WARNING: Inno Setup (iscc) not found on PATH. Skipping installer build." -ForegroundColor Yellow
    Write-Host "  Install Inno Setup 6+ from https://jrsoftware.org/isdl.php and re-run."
}

Write-Host "`n=== Release build complete ===" -ForegroundColor Green
