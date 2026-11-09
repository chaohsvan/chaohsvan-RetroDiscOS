#!/usr/bin/env bash
# RetroDisc session launcher. systemd runs this on tty1; the player owns the VT.
#
# Display strategy:
#   1. If an X/Wayland server is already running, use it (development / VM).
#   2. Otherwise use SDL's kmsdrm backend on the bare VT (target device).
#
# The player is exec'd, so its exit status is what systemd sees and
# Restart=on-failure can do its job.
set -euo pipefail

BIN="${RETRODISC_BIN:-/opt/retrodisc/bin/retrodisc}"
CONFIG="${RETRODISC_CONFIG:-/opt/retrodisc/etc}"

if [ ! -x "$BIN" ]; then
    echo "retrodisc-session: player binary not found or not executable: $BIN" >&2
    exit 78  # EX_CONFIG: do not restart in a tight loop
fi

if [ ! -d "$CONFIG" ]; then
    echo "retrodisc-session: config directory missing: $CONFIG" >&2
    exit 78
fi

# SDL needs a runtime dir for its own state; systemd sets it via %U normally.
if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
    XDG_RUNTIME_DIR="/run/user/$(id -u)"
    export XDG_RUNTIME_DIR
fi

# Only fall back to KMS/DRM when there is no windowing system to attach to.
if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
fi

# RETRODISC_EXTRA lets a deployment pass extra flags (e.g. "--mock" on a VM
# without a CD drive) without editing this script or the unit file.
# shellcheck disable=SC2086
exec "$BIN" --config "$CONFIG" ${RETRODISC_EXTRA:-} "$@"
