#!/usr/bin/env bash
# Exercises scripts/check_permissions.sh with fake device paths.
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

GOOD="$TMP/sr0"
: > "$GOOD"
chmod 0644 "$GOOD"

BAD="$TMP/sr1"
: > "$BAD"
chmod 0000 "$BAD"

# Readable CD, no audio node: success with a warning.
set +e
out=$(RETRODISC_CD="$GOOD" RETRODISC_SND="$TMP/no-such-snd" "$ROOT/scripts/check_permissions.sh")
status=$?
set -e
[ "$status" -eq 0 ] || fail "readable CD should pass, got $status"
echo "$out" | grep -q "\[ok\].*CD device" || fail "missing CD ok line"
echo "$out" | grep -q "\[warn\].*no audio device" || fail "missing audio warning"

# Unreadable CD: must fail with a concrete fix.
set +e
out=$(RETRODISC_CD="$BAD" RETRODISC_SND="$TMP/no-such-snd" "$ROOT/scripts/check_permissions.sh")
status=$?
set -e
[ "$status" -eq 1 ] || fail "unreadable CD should fail, got $status"
echo "$out" | grep -q "usermod -aG cdrom" || fail "should suggest the group fix"

# Missing CD entirely: warning only, still exit 0 (mock mode is valid).
set +e
out=$(RETRODISC_CD="$TMP/absent" RETRODISC_SND="$TMP/no-such-snd" "$ROOT/scripts/check_permissions.sh")
status=$?
set -e
[ "$status" -eq 0 ] || fail "missing CD should not fail"
echo "$out" | grep -q "\[warn\].*no CD device" || fail "missing CD warning"

echo "permissions check: OK"
