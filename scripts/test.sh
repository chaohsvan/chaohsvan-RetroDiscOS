#!/usr/bin/env bash
# Build then run the full test suite. Usage: scripts/test.sh [Debug|Release]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

"$(dirname "${BASH_SOURCE[0]}")/build.sh" "${1:-Debug}"
ctest --test-dir build --output-on-failure
