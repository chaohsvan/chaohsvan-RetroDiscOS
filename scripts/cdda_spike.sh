#!/usr/bin/env bash
# CD-DA feasibility spike for milestone M4 (T23).
#
# Answers one question: can this drive deliver reliable digital audio?
# Writes a markdown report you can paste into docs/hardware_test.md.
set -uo pipefail

DEV="/dev/sr0"
RIP=0
KEEP=""

usage() {
    cat <<'EOF'
Usage: scripts/cdda_spike.sh [-d DEVICE] [--rip]

  -d DEVICE   block device (default /dev/sr0)
  --rip       also rip track 1 and analyse the PCM (slower, needs ~50 MB)

Exit codes: 0 = spike passed, 2 = no drive, 3 = no medium, 4 = read problem.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        -d) DEV="$2"; shift 2 ;;
        --rip) RIP=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "cdda_spike: unknown option: $1" >&2; exit 2 ;;
    esac
done

say() { echo "$@"; }
hdr() { echo; echo "## $*"; }

say "# CD-DA spike report"
say ""
say "- device: \`$DEV\`"
say "- time: $(date -Is)"

if [ ! -e "$DEV" ]; then
    say ""
    say "**FAIL: device node does not exist.** Is the drive plugged in?"
    exit 2
fi

if ! command -v cdparanoia >/dev/null 2>&1; then
    say ""
    say "**FAIL: cdparanoia is not installed.**  apt install cdparanoia"
    exit 2
fi

hdr "Drive"
if [ -r "/sys/class/block/$(basename "$DEV")/device/model" ]; then
    model=$(cat "/sys/class/block/$(basename "$DEV")/device/model" 2>/dev/null | tr -s ' ')
    say "- model: \`$(cat "/sys/class/block/$(basename "$DEV")/device/vendor" 2>/dev/null | tr -s ' ') ${model}\`"
fi

hdr "Media"
# Read the capacity from sysfs: minimal Debian images do not ship blockdev.
name=$(basename "$DEV")
size=0
if [ -r "/sys/class/block/$name/size" ]; then
    sectors=$(cat "/sys/class/block/$name/size" 2>/dev/null || echo 0)
    size=$((sectors * 512))
elif command -v blockdev >/dev/null 2>&1; then
    size=$(blockdev --getsize64 "$DEV" 2>/dev/null || echo 0)
fi

# Note: an empty slim drive can still report a bogus capacity, so size alone
# does not prove there is a disc. cdparanoia below is the real test.
if [ "$size" -le 0 ]; then
    say "- capacity: 0"
else
    say "- capacity: $((size / 1024 / 1024)) MiB (reported by the drive)"
fi

hdr "TOC (cdparanoia -vsQ)"
toc_out=$("cdparanoia" -vsQ -d "$DEV" 2>&1)
toc_status=$?
say '```'
echo "$toc_out" | tail -30
say '```'

if [ "$toc_status" -ne 0 ]; then
    say ""
    if echo "$toc_out" | grep -qiE "unable to open disc|no medium|no disc|not ready"; then
        say "**No medium.** The drive is there but holds no readable disc."
        say "Push the tray in by hand and try again."
        exit 3
    fi
    say "**FAIL: cdparanoia could not read the TOC (exit $toc_status).**"
    say "This means the disc is in the drive but is not readable as an audio CD."
    exit 4
fi

track_lines=$(echo "$toc_out" | grep -cE "^ *[0-9]+\. ")
say ""
say "- tracks detected: $track_lines"

failures=0
echo "$toc_out" | grep -qi "sector 0.*read error" && failures=$((failures + 1))
echo "$toc_out" | grep -qi "scsi command aborted" && failures=$((failures + 1))

hdr "Rip test"
if [ "$RIP" -eq 1 ]; then
    workdir=$(mktemp -d)
    say "- ripping track 1 into \`$workdir/track01.wav\` …"
    if cdparanoia -w -d "$DEV" 1 "$workdir/track01.wav" > "$workdir/rip.log" 2>&1; then
        say "- cdparanoia exit: 0"
    else
        say "- cdparanoia exit: **non-zero**"
        failures=$((failures + 1))
    fi
    say '```'
    tail -12 "$workdir/rip.log"
    say '```'

    if [ -f "$workdir/track01.wav" ]; then
        python3 - "$workdir/track01.wav" <<'PY'
import sys, wave, struct

path = sys.argv[1]
with wave.open(path, "rb") as wav:
    frames = wav.getnframes()
    rate = wav.getframerate()
    channels = wav.getnchannels()
    width = wav.getsampwidth()
    data = wav.readframes(frames)

samples = struct.unpack("<%dh" % (len(data) // 2), data)
peak = max(abs(s) for s in samples) if samples else 0
silent_run = longest = 0
for s in samples:
    if abs(s) < 8:
        silent_run += 1
        longest = max(longest, silent_run)
    else:
        silent_run = 0

duration = frames / rate if rate else 0
print(f"- format: {rate} Hz / {channels} ch / {width * 8} bit")
print(f"- duration: {duration:.2f} s")
print(f"- peak: {peak} ({peak / 32768 * 100:.1f} % of full scale)")
print(f"- longest near-silent run: {longest / rate:.3f} s" if rate else "")
threshold = rate * 0.05 if rate else 1
if peak == 0:
    print("- **FAIL: track is digital silence**")
elif longest > threshold:
    print(f"- **WARN: a {(longest / rate):.3f} s silent gap looks like a read dropout**")
else:
    print("- no suspicious silent gaps")
PY
    else
        say "- **FAIL: no WAV produced**"
        failures=$((failures + 1))
    fi
    KEEP="$workdir"
else
    say "- skipped (pass --rip to analyse the PCM)"
fi

hdr "Verdict"
if [ "$failures" -eq 0 ]; then
    say "- **PASS**: the drive reads CD-DA and the TOC is sane."
    say "- Next: implement \`RealCDBackend\` on libcdio-paranoia (T24)."
else
    say "- **FAIL** ($failures problem(s)). Do not write RealCDBackend yet;"
    say "  try another disc before blaming the drive."
fi

if [ -n "$KEEP" ]; then
    say ""
    say "- ripped audio kept in \`$KEEP\` for listening"
fi

[ "$failures" -eq 0 ] || exit 4
exit 0
