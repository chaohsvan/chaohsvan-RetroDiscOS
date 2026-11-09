#!/usr/bin/env bash
# Reports the storage/power settings that keep the appliance quiet and stable:
#   - /tmp on tmpfs (no SSD writes for temp files)
#   - journald size limits applied
#   - USB storage/audio devices not on autosuspend
#
# Exit 0 unless --strict is given and a real problem is found.
set -uo pipefail

STRICT=0
[ "${1:-}" = "--strict" ] && STRICT=1

problems=0
shopt -s nullglob

echo "RetroDisc power/storage check"

# --- /tmp -------------------------------------------------------------------
tmp_type=$(findmnt -no FSTYPE /tmp 2>/dev/null || echo "none")
if [ "$tmp_type" = "tmpfs" ]; then
    echo "  [ok]   /tmp is tmpfs ($(findmnt -no SIZE /tmp 2>/dev/null))"
else
    echo "  [warn] /tmp is '$tmp_type', not tmpfs"
    problems=$((problems + 1))
fi

# --- journald ---------------------------------------------------------------
max_use=$(systemd-analyze cat-config systemd/journald.conf 2>/dev/null |
          awk -F= '/^SystemMaxUse=/ {print $2; exit}')
if [ -n "${max_use:-}" ]; then
    echo "  [ok]   journald SystemMaxUse=$max_use"
else
    echo "  [warn] journald has no SystemMaxUse limit"
    problems=$((problems + 1))
fi
usage=$(journalctl --disk-usage 2>/dev/null | sed 's/^Archived.*take up //' || echo "unknown")
echo "         current journal usage: ${usage:-unknown}"

# --- USB power --------------------------------------------------------------
sleeping=0
for dev in /sys/bus/usb/devices/*; do
    [ -r "$dev/idVendor" ] || continue
    vid=$(cat "$dev/idVendor" 2>/dev/null)
    pid=$(cat "$dev/idProduct" 2>/dev/null)
    control=$(cat "$dev/power/control" 2>/dev/null || echo "?")
    classes=""
    for intf in "$dev"/*:*; do
        [ -r "$intf/bInterfaceClass" ] && classes="$classes $(cat "$intf/bInterfaceClass")"
    done
    kind="other"
    case " $classes " in
        *" 08 "*) kind="storage" ;;
        *" 01 "*) kind="audio" ;;
    esac
    if [ "$kind" != "other" ]; then
        if [ "$control" = "auto" ]; then
            echo "  [FAIL] ${vid}:${pid} $kind is on autosuspend (power/control=$control)"
            sleeping=$((sleeping + 1))
        else
            echo "  [ok]   ${vid}:${pid} $kind power/control=$control"
        fi
    fi
done
if [ "$sleeping" -gt 0 ]; then
    problems=$((problems + 1))
    echo "         fix: install 90-retrodisc-usb.rules and run: udevadm control --reload"
fi

if [ "$problems" -eq 0 ]; then
    echo "result: OK"
    exit 0
fi

echo "result: $problems warning(s)"
[ "$STRICT" -eq 1 ] && exit 1
exit 0
