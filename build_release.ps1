# Build a release xmd.exe from the repo.
# This assumes ReXGlue SDK is in sdk/win-amd64 and the game files are in game/.

$ninja = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
$llvm = "C:\Program Files\LLVM\bin"
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$env:PATH = "$ninja;$llvm;$env:PATH"

$project = $PSScriptRoot
$sdkDir = "$project\sdk\win-amd64"

# Regenerate SDK-managed files
& "$sdkDir\bin\rexglue.exe" init --force --project_name xmd --project_root $project --xex_path "$project\game\default.xex" --game_root "$project\game"

# Apply manual generated-code fixes
python "$project\patches\apply_fixes.py"

# Configure and build
& $cmake --preset win-amd64-release -S $project -B "$project\out\build\win-amd64-release" -DCMAKE_PREFIX_PATH="$sdkDir"
& $cmake --build "$project\out\build\win-amd64-release"
