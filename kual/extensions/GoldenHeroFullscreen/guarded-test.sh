#!/bin/sh
# KPW1 FW 5.6.1.1 ONLY: reversible fullscreen guard.
# Do not STOP/kill Xorg or stop framework. Preserve WordScreen/KOReader.
MODE="$1"
case "$MODE" in display|touch|controls|ui|demo|game|standalone) ;; *) exit 2 ;; esac
if [ "$MODE" = standalone ]; then
  LOG_DIR=/mnt/us/golden_hero_standalone/logs
  LOG_NAME=standalone.log
else
  LOG_DIR=/mnt/us/golden_hero/logs
  LOG_NAME="fullscreen-v0.9.6-$MODE.log"
fi
mkdir -p "$LOG_DIR" || exit 3
LOG="$LOG_DIR/$LOG_NAME"
exec >>"$LOG" 2>&1
printf '\n==== Golden Hero KPW1 %s (protected fullscreen) ====\n' "$MODE"
date
if ! grep -q 'Kindle 5\.6\.1\.1' /etc/prettyversion.txt 2>/dev/null; then
  echo 'ERROR: Unsupported firmware. KPW1 5.6.1.1 required. Nothing changed.'
  exit 4
fi
if ! grep -q 'mxc_epdc_fb' /proc/fb 2>/dev/null; then
  echo 'ERROR: Not a supported Kindle framebuffer. Nothing changed.'
  exit 4
fi
find_program() {
  for base in /bin /usr/bin /sbin /usr/sbin; do
    [ -x "$base/$1" ] && { printf '%s\n' "$base/$1"; return 0; }
  done
  return 1
}
KILLALL="$(find_program killall)"
if [ -z "$KILLALL" ]; then
  echo 'ERROR: killall not available. Refusing unsafe fullscreen takeover.'
  exit 4
fi
LIPC="$(find_program lipc-set-prop)"
EXE=/mnt/us/golden_hero/golden_hero
if [ "$MODE" = standalone ]; then
  EXE=/mnt/us/golden_hero_standalone/golden_hero_standalone
fi
if [ "$MODE" != touch ]; then
  if [ ! -x "$EXE" ]; then
    echo "ERROR: Golden Hero KPW1 ARM executable is missing: $EXE"
    exit 5
  fi
else
  LUAJIT=''
  for program in /mnt/us/koreader/luajit /mnt/us/koreader/bin/luajit /mnt/us/extensions/koreader/luajit /mnt/us/extensions/koreader/bin/luajit; do
    [ -x "$program" ] && { LUAJIT="$program"; break; }
  done
  if [ -z "$LUAJIT" ]; then
    echo 'ERROR: KOReader LuaJIT not found; refusing fullscreen takeover.'
    exit 5
  fi
  LUA_HOME="$(dirname "$LUAJIT")"
  export LD_LIBRARY_PATH="$LUA_HOME/libs:$LUA_HOME/lib:$LUA_HOME/../libs:$LUA_HOME:$LD_LIBRARY_PATH"
fi
# One guarded framebuffer owner at a time. Use /tmp so stale locks do not
# survive a Kindle reboot; do not remove another process's lock automatically.
LOCK_DIR=/tmp/golden-hero-kpw1-session.lock
if ! mkdir "$LOCK_DIR" 2>/dev/null; then
  echo 'ERROR: Another Golden Hero session appears active (lock held). Refusing a second framebuffer writer.'
  exit 9
fi
printf "%s\n" "$$" > "$LOCK_DIR/pid"
if [ "$MODE" = standalone ]; then
  GOLDEN_HERO_HEARTBEAT="$LOCK_DIR/heartbeat"
  GOLDEN_HERO_SAVE_DIR=/mnt/us/golden_hero_standalone/saves
  export GOLDEN_HERO_HEARTBEAT GOLDEN_HERO_SAVE_DIR
  if ! mkdir -p "$GOLDEN_HERO_SAVE_DIR"; then
    rm -f "$LOCK_DIR/pid"
    rmdir "$LOCK_DIR"
    exit 3
  fi
fi
if [ "$MODE" = game ]; then
  PRIVATE_ROOT="${GOLDEN_HERO_PRIVATE_DIR:-/mnt/us/documents/GoldenHeroPrivate}"
  export GOLDEN_HERO_PRIVATE_DIR="$PRIVATE_ROOT"
  if [ ! -s "$PRIVATE_ROOT/roms/obj_lu.bin" ] || [ ! -s "$PRIVATE_ROOT/roms/nc1020.fls" ]; then
    echo 'ERROR: Private NC1020 images missing. No fullscreen session started.'
    rm -f "$LOCK_DIR/pid" >/dev/null 2>&1 || true
    rmdir "$LOCK_DIR" >/dev/null 2>&1 || true
    exit 3
  fi
fi
PILLOW_HIDDEN=0
WATCHDOG_PID=''
GAME_PID=''
GAME_WATCHDOG_PID=''
restore_ui() {
  echo 'Restoring Kindle UI and status bar...'
  if [ -n "$GAME_PID" ]; then
    kill -TERM "$GAME_PID" >/dev/null 2>&1 || true
    sleep 1
    kill -KILL "$GAME_PID" >/dev/null 2>&1 || true
    wait "$GAME_PID" >/dev/null 2>&1 || true
    GAME_PID=''
  fi
  if [ -n "$GAME_WATCHDOG_PID" ]; then
    kill "$GAME_WATCHDOG_PID" >/dev/null 2>&1 || true
    wait "$GAME_WATCHDOG_PID" >/dev/null 2>&1 || true
  fi
  "$KILLALL" -CONT awesome >/dev/null 2>&1 || true
  if [ "$PILLOW_HIDDEN" = 1 ] && [ -n "$LIPC" ]; then
    "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.showMe();"}' >/dev/null 2>&1 || true
  fi
  if [ -n "$WATCHDOG_PID" ]; then
    kill "$WATCHDOG_PID" >/dev/null 2>&1 || true
    wait "$WATCHDOG_PID" >/dev/null 2>&1 || true
  fi
  if [ -d "$LOCK_DIR" ]; then
    rm -f "$LOCK_DIR/pid" "$LOCK_DIR/heartbeat" "$LOCK_DIR/child" >/dev/null 2>&1 || true
    rmdir "$LOCK_DIR" >/dev/null 2>&1 || true
  fi
  echo 'UI restoration attempted.'
}
trap 'restore_ui' 0
trap 'exit 129' 1
trap 'exit 130' 2
trap 'exit 131' 3
trap 'exit 143' 15
owns_screen() {
  [ -f "$LOCK_DIR/pid" ] && IFS= read -r owner < "$LOCK_DIR/pid" && [ "$owner" = "$$" ]
}
recover_screen() {
  "$KILLALL" -CONT awesome >/dev/null 2>&1 || true
  if [ -n "$LIPC" ]; then
    "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.showMe();"}' >/dev/null 2>&1 || true
  fi
}
if [ "$MODE" = standalone ]; then
  (
    trap - 0
    trap 'exit 0' 1 2 3 15
    stale=0
    previous=''
    while owns_screen && kill -0 "$$" 2>/dev/null; do
      sleep 2
      beat=''
      if [ -f "$GOLDEN_HERO_HEARTBEAT" ]; then
        IFS= read -r beat < "$GOLDEN_HERO_HEARTBEAT" || true
      fi
      if [ -n "$beat" ] && [ "$beat" != "$previous" ]; then
        previous="$beat"
        stale=0
      else
        stale=$((stale + 1))
      fi
      [ "$stale" -lt 12 ] || break
    done
    owns_screen || exit 0
    echo 'Standalone watchdog: launcher gone or game stopped responding.'
    kill -TERM "$$" 2>/dev/null || true
    sleep 4
    owns_screen || exit 0
    child=''
    if [ -f "$LOCK_DIR/child" ]; then IFS= read -r child < "$LOCK_DIR/child" || true; fi
    case "$child" in
      ''|*[!0-9]*|0|1) ;;
      *)
        kill -TERM "$child" 2>/dev/null || true
        sleep 2
        kill -KILL "$child" 2>/dev/null || true
        ;;
    esac
    owns_screen || exit 0
    recover_screen
    rm -f "$LOCK_DIR/pid" "$LOCK_DIR/heartbeat" "$LOCK_DIR/child"
    rmdir "$LOCK_DIR" || true
  ) </dev/null &
else
  (
    trap '' 1 2 3 15
    sleep 65
    recover_screen
  ) </dev/null >/dev/null 2>&1 &
fi
WATCHDOG_PID=$!
echo "Recovery watchdog PID: $WATCHDOG_PID"
if [ -n "$LIPC" ]; then
  if "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.hideMe();"}' >/dev/null 2>&1; then
    PILLOW_HIDDEN=1
    echo 'Pillow status bar requested hidden.'
  else
    echo 'WARN: Pillow hide command was rejected; continuing with WM guard.'
  fi
fi
if "$KILLALL" -STOP awesome >/dev/null 2>&1; then
  echo 'Awesome window manager paused; the test now owns the framebuffer.'
else
  echo 'ERROR: Unable to pause Awesome. Avoid drawing over active Kindle GUI.'
  exit 6
fi
sleep 2
if [ "$MODE" = display ]; then
  echo 'Starting KPW1 ARM display test.'
  "$EXE" --display-test
  RET=$?
elif [ "$MODE" = controls ]; then
  echo 'Starting no-ROM touch controls test (45-second maximum).'
  "$EXE" --controls-test
  RET=$?
elif [ "$MODE" = ui ]; then
  echo 'Starting no-ROM 160x80 LCD and touch layout preview (45-second maximum).'
  "$EXE" --ui-test
  RET=$?
elif [ "$MODE" = demo ]; then
  echo 'Starting ROM-free miniature town walking and interaction test (45-second maximum).'
  "$EXE" --demo-test
  RET=$?
elif [ "$MODE" = standalone ]; then
  echo 'Starting standalone Golden Hero (heartbeat monitored).'
  "$EXE" --protected-session &
  GAME_PID=$!
  printf '%s\n' "$GAME_PID" > "$LOCK_DIR/child"
  wait "$GAME_PID"
  RET=$?
  GAME_PID=''
elif [ "$MODE" = game ]; then
  echo 'Starting protected REAL NC1020 AUTO-GAME test: 30s; synthetic F11; MAIN/SAVE disabled; no state/Flash writes.'
  "$EXE" --safe-game-test &
  GAME_PID=$!
  # Independent recovery if the fullscreen child fails to exit.
  (
    trap '' 1 2 3 15
    sleep 45
    kill -TERM "$GAME_PID" >/dev/null 2>&1 || true
    sleep 3
    kill -KILL "$GAME_PID" >/dev/null 2>&1 || true
    "$KILLALL" -CONT awesome >/dev/null 2>&1 || true
    if [ -n "$LIPC" ]; then
      "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.showMe();"}' >/dev/null 2>&1 || true
    fi
  ) </dev/null >/dev/null 2>&1 &
  GAME_WATCHDOG_PID=$!
  wait "$GAME_PID"
  RET=$?
  GAME_PID=''
else
  echo 'Starting raw cyttsp input event capture.'
  "$LUAJIT" /mnt/us/golden_hero_diagnostics/raw-touch.lua
  RET=$?
fi
printf 'Test process exit code: %s\n' "$RET"
exit "$RET"
