#!/usr/bin/env bash
# Installs the player into a prefix. Never touches systemd by itself: it only
# places files, so it is safe to run without root against a private prefix.
set -euo pipefail

PREFIX="${RETRODISC_PREFIX:-/opt/retrodisc}"
BUILD_DIR="build"
UNIT_DIR=""
TMPFILES_DIR=""
INSTALL_CONFIG=1

usage() {
    cat <<'EOF'
Usage: scripts/deploy.sh [options]

  --prefix DIR      install prefix (default /opt/retrodisc)
  --build DIR       build directory (default build)
  --unit-dir DIR    also copy retrodisc.service here (e.g. /etc/systemd/system)
  --tmpfiles-dir DIR  also copy the tty tmpfiles rule here (e.g. /etc/tmpfiles.d)
  --skip-config     do not install config files
  -h, --help        show this help

The previous binary is kept under <prefix>/previous for scripts/rollback.sh.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix) PREFIX="$2"; shift 2 ;;
        --build) BUILD_DIR="$2"; shift 2 ;;
        --unit-dir) UNIT_DIR="$2"; shift 2 ;;
        --tmpfiles-dir) TMPFILES_DIR="$2"; shift 2 ;;
        --skip-config) INSTALL_CONFIG=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "deploy: unknown option: $1" >&2; exit 2 ;;
    esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$BUILD_DIR/retrodisc"
if [ ! -x "$BIN" ]; then
    echo "deploy: $BIN not found or not executable; run scripts/build.sh first" >&2
    exit 1
fi

mkdir -p "$PREFIX/bin" "$PREFIX/etc" "$PREFIX/lib/systemd/system" "$PREFIX/previous/bin"

# Rotate the current binary so rollback has something to return to.
if [ -x "$PREFIX/bin/retrodisc" ]; then
    cp -f "$PREFIX/bin/retrodisc" "$PREFIX/previous/bin/retrodisc"
    echo "deploy: kept previous binary for rollback"
fi

install -m 0755 "$BIN" "$PREFIX/bin/retrodisc"
install -m 0755 "$ROOT/system/retrodisc-session.sh" "$PREFIX/bin/retrodisc-session"

# Config is user data: never overwrite an existing file.
if [ "$INSTALL_CONFIG" -eq 1 ]; then
    for conf in "$ROOT"/config/*.conf; do
        target="$PREFIX/etc/$(basename "$conf")"
        if [ -e "$target" ]; then
            echo "deploy: keeping existing $target"
        else
            install -m 0644 "$conf" "$target"
            echo "deploy: installed $target"
        fi
    done
fi

install -m 0644 "$ROOT/system/retrodisc.service" "$PREFIX/lib/systemd/system/retrodisc.service"
mkdir -p "$PREFIX/lib/tmpfiles.d"
install -m 0644 "$ROOT/system/retrodisc-tmpfiles.conf" "$PREFIX/lib/tmpfiles.d/retrodisc.conf"
if [ -n "$UNIT_DIR" ]; then
    mkdir -p "$UNIT_DIR"
    install -m 0644 "$ROOT/system/retrodisc.service" "$UNIT_DIR/retrodisc.service"
    echo "deploy: unit copied to $UNIT_DIR"
fi
if [ -n "$TMPFILES_DIR" ]; then
    mkdir -p "$TMPFILES_DIR"
    install -m 0644 "$ROOT/system/retrodisc-tmpfiles.conf" "$TMPFILES_DIR/retrodisc.conf"
    echo "deploy: tmpfiles rule copied to $TMPFILES_DIR"
fi

{
    echo "version=$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "installed=$(date -Is)"
} > "$PREFIX/version"

echo "deploy: installed into $PREFIX"
if [ -z "$UNIT_DIR" ]; then
    echo "deploy: next steps (as root):"
    echo "  install -m 0644 $PREFIX/lib/systemd/system/retrodisc.service /etc/systemd/system/"
    echo "  systemd-tmpfiles --create retrodisc.conf"
    echo "  systemctl daemon-reload && systemctl enable --now retrodisc"
fi
