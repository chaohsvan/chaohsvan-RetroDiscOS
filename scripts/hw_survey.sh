#!/usr/bin/env bash
# Collects the hardware facts RetroDisc OS needs (M4/M5 prerequisites).
# Read-only. Prints a markdown report to paste into docs/hardware_test.md.
set -uo pipefail

na() { echo "N/A"; }

section() { echo; echo "## $*"; }

first_line() { head -n 1 "$1" 2>/dev/null | tr -s ' ' || na; }

echo "# 硬件普查报告"
echo
echo "- 时间: $(date -Is)"

section "系统"
if [ -r /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    echo "- 发行版: ${PRETTY_NAME:-unknown}"
fi
echo "- 内核: $(uname -r)"
echo "- 架构: $(uname -m)"
echo "- 主机名: $(hostname)"
echo "- 内存: $(awk '/MemTotal/ {printf "%.1f GiB", $2/1024/1024}' /proc/meminfo 2>/dev/null || na)"

section "CPU"
model=$(awk -F: '/model name/ {gsub(/^ +/, "", $2); print $2; exit}' /proc/cpuinfo 2>/dev/null)
[ -n "${model:-}" ] || model=$(na)
echo "- 型号: \`$model\`"
echo "- 逻辑核数: $(nproc 2>/dev/null || na)"
governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)
echo "- 调频策略: ${governor:-$(na)}"
driver=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver 2>/dev/null)
echo "- 调频驱动: ${driver:-$(na)}"

section "显示与背光"
if compgen -G "/sys/class/backlight/*" > /dev/null; then
    for dir in /sys/class/backlight/*; do
        name=$(basename "$dir")
        max=$(cat "$dir/max_brightness" 2>/dev/null || na)
        cur=$(cat "$dir/brightness" 2>/dev/null || na)
        echo "- \`$name\`: brightness=$cur / max=$max"
    done
else
    echo "- 无 backlight 设备（笔记本通常有，台式机没有）"
fi
if compgen -G "/dev/dri/card*" > /dev/null; then
    echo "- DRM 设备: $(ls /dev/dri 2>/dev/null | tr '\n' ' ')"
else
    echo "- 无 /dev/dri（kmsdrm 不可用）"
fi
connected=$(grep -l "^connected" /sys/class/drm/*/status 2>/dev/null | wc -l)
echo "- 已连接显示输出: ${connected:-0}"

section "电源与 ACPI"
if compgen -G "/sys/class/power_supply/*" > /dev/null; then
    for dir in /sys/class/power_supply/*; do
        name=$(basename "$dir")
        type=$(cat "$dir/type" 2>/dev/null || na)
        online=$(cat "$dir/online" 2>/dev/null || echo "-")
        cap=$(cat "$dir/capacity" 2>/dev/null || echo "-")
        echo "- \`$name\`: type=$type online=$online capacity=$cap"
    done
else
    echo "- 无 power_supply 设备"
fi
echo "- logind HandlePowerKey: $(grep -h '^HandlePowerKey' /etc/systemd/logind.conf /etc/systemd/logind.conf.d/*.conf 2>/dev/null | tail -1 || echo 'default (poweroff)')"

section "温度与风扇"
if compgen -G "/sys/class/thermal/thermal_zone*" > /dev/null; then
    for dir in /sys/class/thermal/thermal_zone*; do
        type=$(cat "$dir/type" 2>/dev/null || na)
        temp=$(cat "$dir/temp" 2>/dev/null || echo 0)
        echo "- $type: $((temp / 1000)) °C"
    done
else
    echo "- 无 thermal zone"
fi
fan_found=0
for dir in /sys/class/hwmon/hwmon*; do
    [ -d "$dir" ] || continue
    for f in "$dir"/fan*_input; do
        [ -e "$f" ] || continue
        fan_found=1
        echo "- $(basename "$dir")/$(basename "$f"): $(cat "$f" 2>/dev/null) RPM"
    done
done
[ "$fan_found" -eq 1 ] || echo "- 未暴露风扇转速（只监测，不控制）"

section "光驱"
if [ -e /dev/sr0 ]; then
    vendor=$(cat /sys/class/block/sr0/device/vendor 2>/dev/null | tr -s ' ')
    model=$(cat /sys/class/block/sr0/device/model 2>/dev/null | tr -s ' ')
    echo "- \`/dev/sr0\`: $vendor $model"
    size=$(blockdev --getsize64 /dev/sr0 2>/dev/null || echo "")
    if [ -n "$size" ]; then
        echo "- 介质容量: $((size / 1024 / 1024)) MiB"
    else
        echo "- 介质: 空（或未插入）"
    fi
    lsblk -o NAME,SIZE,TYPE,TRAN 2>/dev/null | grep -E "sr0|NAME" | sed 's/^/    /'
else
    echo "- 无 /dev/sr0"
fi

section "音频"
if [ -r /proc/asound/cards ]; then
    echo '```'
    cat /proc/asound/cards
    echo '```'
else
    echo "- 无 /proc/asound（没有 ALSA）"
fi
echo "- /dev/snd 条目: $(ls /dev/snd 2>/dev/null | tr '\n' ' ' || echo 无)"

section "输入设备"
echo "- event 设备数: $(ls /dev/input/event* 2>/dev/null | wc -l)"
grep -E "^N: Name|^H: Handlers" /proc/bus/input/devices 2>/dev/null | head -12 | sed 's/^/    /'

section "存储"
df -h / 2>/dev/null | tail -1 | sed 's/^/    /'

section "结论提示"
echo "- 背光可用 → M5 可以真正调屏幕亮度"
echo "- 无 /dev/dri → kmsdrm 全屏不可用，需要 X 或其他方案"
echo "- 有 /dev/sr0 → CD-DA spike 直接在这台跑（scripts/cdda_spike.sh）"
