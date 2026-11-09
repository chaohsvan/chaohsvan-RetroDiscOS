#!/usr/bin/env bash
# T32/T33: journald limits, USB power rules, and the check script.
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
RULES="$ROOT/system/90-retrodisc-usb.rules"
BACKLIGHT_RULES="$ROOT/system/85-retrodisc-backlight.rules"
JOURNAL="$ROOT/system/retrodisc-journald.conf"
CHECK="$ROOT/scripts/check_power.sh"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

[ -f "$RULES" ] || fail "missing $RULES"
[ -f "$JOURNAL" ] || fail "missing $JOURNAL"
[ -x "$CHECK" ] || fail "check_power.sh is not executable"

# --- journald contract -------------------------------------------------------
for key in "Storage=" "SystemMaxUse=" "SystemMaxFileSize=" "MaxRetentionSec="; do
    grep -q "^$key" "$JOURNAL" || fail "journald config missing $key"
done

# --- udev rules --------------------------------------------------------------
grep -q "08\*" "$RULES" || fail "rules must cover USB mass storage (class 08)"
grep -q "01\*" "$RULES" || fail "rules must cover USB audio (class 01)"
grep -q "power/control}=\"on\"" "$RULES" || fail "rules must force power/control=on"

[ -f "$BACKLIGHT_RULES" ] || fail "missing $BACKLIGHT_RULES"
grep -q "video" "$BACKLIGHT_RULES" || fail "backlight rule must target the video group"
grep -q "chmod g+w" "$BACKLIGHT_RULES" || fail "backlight rule must make the file group-writable"

if command -v udevadm > /dev/null 2>&1; then
    if ! udevadm verify "$BACKLIGHT_RULES" > /tmp/udev-verify-bl.log 2>&1; then
        cat /tmp/udev-verify-bl.log >&2
        fail "udevadm verify rejected the backlight rules"
    fi
    rm -f /tmp/udev-verify-bl.log
    if ! udevadm verify "$RULES" > /tmp/udev-verify.log 2>&1; then
        cat /tmp/udev-verify.log >&2
        fail "udevadm verify rejected the rules"
    fi
    rm -f /tmp/udev-verify.log
fi

# --- check script ------------------------------------------------------------
out=$("$CHECK" 2>&1) || true
echo "$out" | grep -q "^RetroDisc power/storage check" || fail "check script header missing"
echo "$out" | grep -q "tmp" || fail "check script should mention /tmp"
echo "$out" | grep -qE "journald" || fail "check script should mention journald"

# --strict must still work (exit code depends on the machine, so only assert it runs).
"$CHECK" --strict > /dev/null 2>&1 || true

echo "power/storage: OK"
