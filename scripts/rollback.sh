#!/usr/bin/env bash
# Restores the binary kept by scripts/deploy.sh.
set -euo pipefail

PREFIX="${RETRODISC_PREFIX:-/opt/retrodisc}"

usage() {
    cat <<'EOF'
Usage: scripts/rollback.sh [--prefix DIR]

Swaps <prefix>/bin/retrodisc with <prefix>/previous/bin/retrodisc.
Config files are never touched.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix) PREFIX="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "rollback: unknown option: $1" >&2; exit 2 ;;
    esac
done

CURRENT="$PREFIX/bin/retrodisc"
PREVIOUS="$PREFIX/previous/bin/retrodisc"

if [ ! -x "$PREVIOUS" ]; then
    echo "rollback: no previous binary at $PREVIOUS" >&2
    exit 1
fi

mkdir -p "$PREFIX/previous/bin"
if [ -x "$CURRENT" ]; then
    cp -f "$CURRENT" "$PREFIX/previous/bin/retrodisc.new"
fi

cp -f "$PREVIOUS" "$CURRENT"
chmod 0755 "$CURRENT"

if [ -x "$PREFIX/previous/bin/retrodisc.new" ]; then
    mv -f "$PREFIX/previous/bin/retrodisc.new" "$PREVIOUS"
fi

echo "rollback: restored $CURRENT from $PREVIOUS"
if [ -f "$PREFIX/version" ]; then
    echo "rollback: note that $PREFIX/version still describes the replaced build"
fi
