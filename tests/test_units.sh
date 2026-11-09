#!/usr/bin/env bash
# Validates the systemd unit and the session launcher without needing a VM.
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
UNIT="$ROOT/system/retrodisc.service"
SESSION="$ROOT/system/retrodisc-session.sh"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

require_key() {
    grep -qE "^$1" "$UNIT" || fail "unit is missing $1"
}

[ -f "$UNIT" ] || fail "missing $UNIT"
[ -f "$SESSION" ] || fail "missing $SESSION"

# --- unit contract -----------------------------------------------------------
require_key "Restart=on-failure"
require_key "RestartSec=3"
require_key "User=retrodisc"
require_key "TTYPath=/dev/tty1"
require_key "WantedBy=multi-user.target"
require_key "NoNewPrivileges=yes"
require_key "ProtectSystem=strict"
require_key "ReadWritePaths=/home/retrodisc/.local/state"
require_key "DeviceAllow=/dev/tty1 rw"   # StandardInput=tty needs this under DevicePolicy=auto
grep -qE "^SupplementaryGroups=.*audio" "$UNIT" || fail "service user needs the audio group"
grep -qE "^SupplementaryGroups=.*cdrom" "$UNIT" || fail "service user needs the cdrom group"

# The player must not run as root: that is the whole point of the dedicated user.
if grep -qE "^User=root$" "$UNIT"; then
    fail "service must not run as root"
fi

# --- session launcher --------------------------------------------------------
bash -n "$SESSION" || fail "session script has a syntax error"
[ -x "$SESSION" ] || fail "session script is not executable"

# Missing binary: exit 78 (EX_CONFIG) and a clear message, never a restart loop.
set +e
out=$(RETRODISC_BIN=/nonexistent/retrodisc RETRODISC_CONFIG=/tmp "$SESSION" 2>&1)
status=$?
set -e
[ "$status" -eq 78 ] || fail "missing binary should exit 78, got $status"
echo "$out" | grep -q "not found" || fail "missing binary should be reported"

# Missing config directory: also exit 78.
set +e
RETRODISC_BIN=/bin/true RETRODISC_CONFIG=/nonexistent/config "$SESSION" >/dev/null 2>&1
status=$?
set -e
[ "$status" -eq 78 ] || fail "missing config should exit 78, got $status"

# Healthy path: the script exec's the binary and forwards arguments.
out=$(RETRODISC_BIN=/bin/echo RETRODISC_CONFIG=/tmp "$SESSION" --mock 2>&1)
echo "$out" | grep -q -- "--config /tmp" || fail "script should pass --config to the player"
echo "$out" | grep -q -- "--mock" || fail "script should forward extra arguments"

# Extra flags from the environment are forwarded (used for VM/mock testing).
out=$(RETRODISC_BIN=/bin/echo RETRODISC_CONFIG=/tmp RETRODISC_EXTRA="--mock --seconds 5" "$SESSION" 2>&1)
echo "$out" | grep -q -- "--mock --seconds 5" || fail "RETRODISC_EXTRA should be forwarded"

# Without a windowing system the launcher must select the KMS/DRM backend.
probe=$(mktemp)
printf '#!/bin/sh\necho "SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-unset}"\n' > "$probe"
chmod +x "$probe"
out=$(env -u DISPLAY -u WAYLAND_DISPLAY -u SDL_VIDEODRIVER \
    RETRODISC_BIN="$probe" RETRODISC_CONFIG=/tmp "$SESSION" 2>&1 || true)
echo "$out" | grep -q "SDL_VIDEODRIVER=kmsdrm" || fail "should default to the kmsdrm backend"

# With an X session present it must not override the driver.
out=$(DISPLAY=:0 SDL_VIDEODRIVER= RETRODISC_BIN="$probe" RETRODISC_CONFIG=/tmp "$SESSION" 2>&1 || true)
echo "$out" | grep -q "SDL_VIDEODRIVER=unset" || fail "should not force kmsdrm under X"
rm -f "$probe"

# --- unit syntax -------------------------------------------------------------
if command -v systemd-analyze >/dev/null 2>&1; then
    # The ExecStart path only exists after deployment; that warning is expected.
    verify_output=$(systemd-analyze verify "$UNIT" 2>&1 || true)
    if echo "$verify_output" | grep -v "is not executable" | grep -q "Unknown\|Invalid\|Failed"; then
        echo "$verify_output" >&2
        fail "systemd-analyze reported a problem"
    fi
fi

# tmpfiles rule must grant the tty to the service user.
TMPFILES="$ROOT/system/retrodisc-tmpfiles.conf"
[ -f "$TMPFILES" ] || fail "missing $TMPFILES"
grep -qE "^z /dev/tty1 0620 root tty" "$TMPFILES" || fail "tmpfiles rule must chmod /dev/tty1 0620"

echo "system units: OK"
