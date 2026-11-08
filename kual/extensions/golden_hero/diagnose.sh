#!/bin/sh
# Read-only KPW1 diagnostics. Does not change Kindle power or display settings.
BASE=/mnt/us/golden_hero
OUT="$BASE/logs/kpw1-diagnostic.txt"
mkdir -p "$BASE/logs" 2>/dev/null || exit 1
exec >"$OUT" 2>&1

section() { printf '\n\n===== %s =====\n' "$1"; }
run() {
  printf '\n$'
  for arg in "$@"; do printf ' %s' "$arg"; done
  printf '\n'
  "$@" 2>&1 || printf '[exit=%s]\n' "$?"
}
show_file() {
  [ -r "$1" ] || return 0
  section "$1"
  cat "$1" 2>&1 || true
}

section "KPW1 DEVICE DIAGNOSTIC"
echo "Generated: $(date 2>/dev/null || echo unknown)"
echo "This report is read-only; it does not install files or alter power settings."

section "MODEL / KERNEL"
run uname -a
run getconf LONG_BIT
show_file /etc/prettyversion.txt
show_file /etc/issue
show_file /proc/version
show_file /proc/cpuinfo
show_file /etc/boardid

section "FRAMEBUFFER"
show_file /proc/fb
for f in /sys/class/graphics/fb0/name /sys/class/graphics/fb0/virtual_size \
         /sys/class/graphics/fb0/stride /sys/class/graphics/fb0/bits_per_pixel \
         /sys/class/graphics/fb0/rotate /sys/class/graphics/fb0/blank; do
  show_file "$f"
done
run ls -l /dev/fb0
run ls -l /dev/graphics 2>/dev/null || true

section "INPUT DEVICES"
show_file /proc/bus/input/devices
run ls -l /dev/input 2>/dev/null || true
for d in /sys/class/input/event*; do
  [ -e "$d" ] || continue
  echo "--- $d ---"
  for f in "$d/device/name" "$d/device/uevent" "$d/device/capabilities/abs" \
           "$d/device/capabilities/key"; do
    if [ -r "$f" ]; then echo "[$f]"; cat "$f"; fi
  done
done

section "RUNTIME / LIBRARIES"
run sh -c 'command -v eips; command -v lipc-set-prop; command -v lipc-get-prop; command -v fbset; command -v evtest; command -v readelf; command -v ldd; command -v gcc; command -v g++'
run ls -l /lib/ld-linux* /lib/libc.so* /usr/lib/libstdc++.so* 2>/dev/null || true
if command -v eips >/dev/null 2>&1; then run file "$(command -v eips)" 2>/dev/null || true; fi
if command -v lipc-set-prop >/dev/null 2>&1; then
  section "CURRENT SCREEN-SAVER PROPERTY (READ ONLY)"
  run lipc-get-prop com.lab126.powerd preventScreenSaver
fi

section "STORAGE"
run df -h /mnt/us
run mount

section "PROCESS SNAPSHOT"
run ps

section "END"
echo "Report saved to: $OUT"
echo "Copy this text file from Kindle USB storage and send it back for KPW1-specific tuning."
