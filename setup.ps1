# setup.ps1 - Bootstrap the X-Men Destiny ReXGlue port project.

$ErrorActionPreference = "Stop"

$root   = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdkDir = Join-Path $root "thirdparty\rexglue-sdk"
$tag    = "v0.10.0"

Write-Host "== X-Men Destiny - ReXGlue project setup ==" -ForegroundColor Cyan

foreach ($tool in @("git", "cmake", "ninja", "clang")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Write-Host "MISSING: $tool not found on PATH." -ForegroundColor Red
        Write-Host "  ReXGlue requires: Clang 18+, CMake 3.25+, Ninja, Visual Studio 2022 (Windows SDK)."
        exit 1
    }
}
Write-Host "Prerequisites OK." -ForegroundColor Green

if (Test-Path (Join-Path $sdkDir ".git")) {
    Write-Host "SDK already cloned at $sdkDir"
} else {
    Write-Host "Cloning ReXGlue SDK ($tag) into thirdparty\rexglue-sdk ..."
    git clone --branch $tag --depth 1 https://github.com/rexglue/rexglue-sdk.git $sdkDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

Write-Host "Initializing SDK submodules (this can take a while) ..."
git -C $sdkDir submodule update --init --recursive --depth 1
if ($LASTEXITCODE -ne 0) { throw "submodule init failed" }

Write-Host ""
Write-Host "Setup complete." -ForegroundColor Green
Write-Host "Next steps:"
Write-Host "  1. Extract your Xbox 360 ISO into .\game\ (entrypoint at game\default.xex)"
Write-Host "  2. Build the SDK CLI:  cmake --preset win-amd64-release -DREXSDK_DIR=thirdparty\rexglue-sdk ; cmake --build out\build\win-amd64-release --target rexglue"
Write-Host "  3. Regenerate SDK-managed files:  rexglue init --force --project_name xmd --project_root . --xex_path game\default.xex --game_root game"
Write-Host "  4. Configure & build the port:    cmake --preset win-amd64-relwithdebinfo -DREXSDK_DIR=thirdparty\rexglue-sdk ; cmake --build out\build\win-amd64-relwithdebinfo"
