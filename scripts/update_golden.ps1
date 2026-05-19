# Regenerate test/golden/*.golden.txt from TextTestFixture output.
# Usage (from repo root):
#   .\scripts\update_golden.ps1
# Or via CMake:
#   cmake -S . -B build -DUPDATE_GOLDEN_MASTER=ON
#   cmake --build build --target update-golden

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUPDATE_GOLDEN_MASTER=ON
cmake --build build
cmake --build build --target update-golden
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUPDATE_GOLDEN_MASTER=OFF
Write-Host "Golden files updated under test/golden/. Review diff, then commit."
