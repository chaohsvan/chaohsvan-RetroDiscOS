#!/usr/bin/env bash
# Checks that this user can reach the CD drive and the audio device.
#
# Exit codes: 0 = usable (warnings allowed), 1 = a permission problem was found.
# Devices are configurable so the check is testable without hardware.
set -uo pipefail

CD="${RETRODISC_CD:-/dev/sr0}"
SND="${RETRODISC_SND:-/dev/snd}"
problems=0

echo "RetroDisc access check"
echo "  user:     $(id -un)"
echo "  groups:   $(id -nG)"
echo "  cdrom:    $CD"
echo "  audio:    $SND"

if [ -e "$CD" ]; then
    if [ -r "$CD" ]; then
        echo "  [ok]   CD device is readable"
    else
        echo "  [FAIL] CD device exists but is not readable"
        echo "         fix: sudo usermod -aG cdrom $(id -un)   (then log in again)"
        problems=$((problems + 1))
    fi
else
    echo "  [warn] no CD device at $CD (mock mode still works)"
fi

if [ -d "$SND" ]; then
    if find "$SND" -maxdepth 1 -type c -readable 2>/dev/null | grep -q .; then
        echo "  [ok]   audio devices are readable"
    else
        echo "  [FAIL] $SND exists but no PCM device is readable"
        echo "         fix: sudo usermod -aG audio $(id -un)   (then log in again)"
        problems=$((problems + 1))
    fi
else
    echo "  [warn] no audio device node at $SND"
fi

for group in audio cdrom; do
    if id -nG | tr ' ' '\n' | grep -qx "$group"; then
        echo "  [ok]   member of $group"
    else
        echo "  [warn] not a member of $group"
    fi
done

if [ "$problems" -eq 0 ]; then
    echo "result: OK"
    exit 0
fi

echo "result: FAILED ($problems problem(s))"
exit 1
