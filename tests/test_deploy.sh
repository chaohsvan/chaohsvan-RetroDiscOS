#!/usr/bin/env bash
# Exercises deploy.sh / rollback.sh against a throwaway prefix.
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

PREFIX="$TMP/opt"
BUILD="$TMP/build"
mkdir -p "$BUILD"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

make_binary() {
    printf '#!/bin/sh\necho %s\n' "$1" > "$BUILD/retrodisc"
    chmod +x "$BUILD/retrodisc"
}

make_binary v1
"$ROOT/scripts/deploy.sh" --prefix "$PREFIX" --build "$BUILD" > /dev/null

[ -x "$PREFIX/bin/retrodisc" ] || fail "player binary not installed"
[ -x "$PREFIX/bin/retrodisc-session" ] || fail "session launcher not installed"
[ -f "$PREFIX/lib/systemd/system/retrodisc.service" ] || fail "unit not installed"
[ -f "$PREFIX/etc/player.conf" ] || fail "player.conf not installed"
[ -f "$PREFIX/version" ] || fail "version stamp missing"
[ "$("$PREFIX/bin/retrodisc")" = "v1" ] || fail "installed binary is not v1"

# Config is user data: redeploying must not clobber edits.
echo "# user edit" >> "$PREFIX/etc/player.conf"
"$ROOT/scripts/deploy.sh" --prefix "$PREFIX" --build "$BUILD" > /dev/null
grep -q "# user edit" "$PREFIX/etc/player.conf" || fail "deploy overwrote the user config"

# A second install keeps the previous binary.
make_binary v2
"$ROOT/scripts/deploy.sh" --prefix "$PREFIX" --build "$BUILD" > /dev/null
[ "$("$PREFIX/bin/retrodisc")" = "v2" ] || fail "expected v2 after redeploy"
[ "$("$PREFIX/previous/bin/retrodisc")" = "v1" ] || fail "previous binary was not kept"

# Rollback swaps them back.
"$ROOT/scripts/rollback.sh" --prefix "$PREFIX" > /dev/null
[ "$("$PREFIX/bin/retrodisc")" = "v1" ] || fail "rollback did not restore v1"
[ "$("$PREFIX/previous/bin/retrodisc")" = "v2" ] || fail "rollback should keep the replaced build"

# Rollback without a previous build fails loudly.
FRESH="$TMP/fresh"
set +e
"$ROOT/scripts/rollback.sh" --prefix "$FRESH" > /dev/null 2>&1
status=$?
set -e
[ "$status" -ne 0 ] || fail "rollback without a previous build must fail"

# Deploy without a build must fail instead of installing nothing.
EMPTY="$TMP/empty-build"
mkdir -p "$EMPTY"
set +e
"$ROOT/scripts/deploy.sh" --prefix "$PREFIX" --build "$EMPTY" > /dev/null 2>&1
status=$?
set -e
[ "$status" -ne 0 ] || fail "deploy without a binary must fail"

echo "deploy/rollback: OK"
