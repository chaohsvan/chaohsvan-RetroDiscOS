#!/usr/bin/env bash
# Checks the spike script's argument handling and failure paths.
# The happy path needs a real audio CD and is run by hand (docs/hardware_test.md).
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
SPIKE="$ROOT/scripts/cdda_spike.sh"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

[ -x "$SPIKE" ] || fail "spike script is not executable"

# --help
out=$("$SPIKE" --help)
echo "$out" | grep -q "Usage: scripts/cdda_spike.sh" || fail "--help should print usage"

# unknown option
set +e
"$SPIKE" --bogus > /dev/null 2>&1
status=$?
set -e
[ "$status" -eq 2 ] || fail "unknown option should exit 2, got $status"

# missing device: exit 2 and a clear message
set +e
out=$("$SPIKE" -d /dev/retrodisc-no-such-sr 2>&1)
status=$?
set -e
[ "$status" -eq 2 ] || fail "missing device should exit 2, got $status"
echo "$out" | grep -q "device node does not exist" || fail "missing device message"
echo "$out" | grep -q "^# CD-DA spike report" || fail "report header missing"

# The report must be valid markdown that can be pasted into the docs.
echo "$out" | grep -q "^- device: " || fail "report should list the device"

echo "cdda spike: OK"
