#!/usr/bin/env bash
# Configure and build. Usage: scripts/build.sh [Debug|Release]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

BUILD_TYPE="${1:-${BUILD_TYPE:-Debug}}"

cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build -j"$(nproc)"
