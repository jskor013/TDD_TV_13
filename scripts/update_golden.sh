#!/usr/bin/env bash
# Regenerate test/golden/*.golden.txt from TextTestFixture output.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUPDATE_GOLDEN_MASTER=ON
cmake --build build
cmake --build build --target update-golden
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUPDATE_GOLDEN_MASTER=OFF
echo "Golden files updated under test/golden/. Review diff, then commit."
