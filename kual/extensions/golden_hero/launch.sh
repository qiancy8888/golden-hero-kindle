#!/bin/sh
# KUAL launcher for the KPW1 Golden Hero port.
cd /mnt/us/golden_hero || exit 1
mkdir -p data logs

if [ ! -x ./golden_hero ]; then
  echo "Missing executable /mnt/us/golden_hero/golden_hero" >> logs/launcher.log
  exit 2
fi
if [ ! -s roms/obj_lu.bin ] || [ ! -s roms/nc1020.fls ]; then
  echo "Game images missing. Read roms/READ_ME.txt; no ROM is bundled." >> logs/launcher.log
  exit 3
fi

# Resolve LIPC binaries without relying on `command -v` (not present in
# some legacy Kindle /bin/sh builds).
find_program() {
  wanted="$1"
  old_ifs="$IFS"
  IFS=:
  for dir in $PATH; do
    [ -n "$dir" ] || dir="."
    if [ -x "$dir/$wanted" ]; then
      IFS="$old_ifs"
      printf '%s\n' "$dir/$wanted"
      return 0
    fi
  done
  IFS="$old_ifs"
  for dir in /usr/bin /usr/sbin /bin /sbin; do
    if [ -x "$dir/$wanted" ]; then
      printf '%s\n' "$dir/$wanted"
      return 0
    fi
  done
  return 1
}

# Temporarily inhibit the Kindle screensaver while the game is in the foreground.
# Preserve the previous value and restore it on normal exit or handled signals.
LIPC_GET="$(find_program lipc-get-prop 2>/dev/null || true)"
LIPC_SET="$(find_program lipc-set-prop 2>/dev/null || true)"
PREVIOUS_SCREEN_SAVER=""
SCREEN_SAVER_CHANGED=0
if [ -n "$LIPC_GET" ] && [ -n "$LIPC_SET" ]; then
  PREVIOUS_SCREEN_SAVER=$("$LIPC_GET" com.lab126.powerd preventScreenSaver 2>/dev/null | tr -d '[]\r\n ')
  case "$PREVIOUS_SCREEN_SAVER" in
    0)
      if "$LIPC_SET" -i com.lab126.powerd preventScreenSaver 1 >/dev/null 2>&1; then
        SCREEN_SAVER_CHANGED=1
      else
        echo "Warning: could not inhibit Kindle screensaver via LIPC." >> logs/launcher.log
      fi
      ;;
    1)
      # It was already inhibited; leave it alone.
      ;;
    *)
      echo "Warning: could not read original preventScreenSaver value; leaving it unchanged." >> logs/launcher.log
      ;;
  esac
else
  echo "Warning: LIPC get/set pair unavailable; screensaver inhibit not changed." >> logs/launcher.log
fi
restore_screen_saver() {
  if [ "$SCREEN_SAVER_CHANGED" = "1" ] && [ -n "$LIPC_SET" ]; then
    if [ -n "$PREVIOUS_SCREEN_SAVER" ]; then
      "$LIPC_SET" -i com.lab126.powerd preventScreenSaver "$PREVIOUS_SCREEN_SAVER" >/dev/null 2>&1 || true
    else
      "$LIPC_SET" -i com.lab126.powerd preventScreenSaver 0 >/dev/null 2>&1 || true
    fi
  fi
}
trap restore_screen_saver EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

./golden_hero >> logs/latest.log 2>&1
rc=$?
echo "Emulator exited with $rc" >> logs/launcher.log
exit "$rc"
