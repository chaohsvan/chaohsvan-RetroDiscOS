#!/usr/bin/env bash
# Smoke test for scripts/hw_survey.sh: it must always produce a report.
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
SURVEY="$ROOT/scripts/hw_survey.sh"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

[ -x "$SURVEY" ] || fail "survey script is not executable"

out=$("$SURVEY" 2>/dev/null) || fail "survey script exited non-zero"
echo "$out" | grep -q "^# 硬件普查报告" || fail "missing report header"
for section in "## 系统" "## CPU" "## 显示与背光" "## 光驱" "## 音频" "## 存储"; do
    echo "$out" | grep -qF "$section" || fail "missing section: $section"
done

# It must degrade gracefully rather than print nothing for absent hardware.
echo "$out" | grep -qE "无 backlight|brightness=" || fail "backlight line missing"

echo "hw survey: OK"
