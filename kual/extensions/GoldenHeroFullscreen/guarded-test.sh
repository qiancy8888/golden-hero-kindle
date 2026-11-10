#!/bin/sh
# KPW1 FW 5.6.1.1 ONLY: reversible fullscreen guard.
# Do not STOP/kill Xorg or stop framework. Preserve WordScreen/KOReader.
MODE="$1"
case "$MODE" in display|touch|controls|ui|demo|game|standalone|platinum) ;; *) exit 2 ;; esac
RECOVERY_SECONDS=65
case "$MODE" in
  standalone)
    LOG_DIR=/mnt/us/golden_hero_standalone/logs
    LOG_NAME=opening-v0.1.0.log ;;
  platinum)
    PLATINUM_ROOT=/mnt/us/platinum_hero
    LOG_DIR="$PLATINUM_ROOT/logs"
    LOG_NAME=platinum.log ;;
  *)
    LOG_DIR=/mnt/us/golden_hero/logs
    LOG_NAME="fullscreen-v0.9.6-$MODE.log" ;;
esac
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
case "$MODE" in
  standalone) EXE=/mnt/us/golden_hero_standalone/golden_hero_standalone ;;
  platinum) EXE="$PLATINUM_ROOT/platinum_hero" ;;
esac
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
if [ "$MODE" = platinum ] && ! "$EXE" --verify-rom "$PLATINUM_ROOT/hero.gbc"; then
  echo 'ERROR: White Platinum ROM validation failed. No fullscreen session started.'
  exit 3
fi
# One guarded framebuffer owner at a time. Use /tmp so stale locks do not
# survive a Kindle reboot; do not remove another process's lock automatically.
LOCK_DIR=/tmp/golden-hero-kpw1-session.lock
if ! mkdir "$LOCK_DIR" 2>/dev/null; then
  echo 'ERROR: Another Golden Hero session appears active (lock held). Refusing a second framebuffer writer.'
  exit 9
fi
if ! printf '%s\n' "$$" > "$LOCK_DIR/pid"; then
  rmdir "$LOCK_DIR" 2>/dev/null || true
  exit 9
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
SCREENSAVER_BEFORE=''
if [ "$MODE" = platinum ] && [ -n "$LIPC" ]; then
  LIPC_GET="$(find_program lipc-get-prop)"
  if [ -n "$LIPC_GET" ] && value="$("$LIPC_GET" com.lab126.powerd preventScreenSaver 2>/dev/null)"; then
    case "$value" in 0|1) SCREENSAVER_BEFORE="$value" ;; esac
  fi
fi
watchdog_sleep() {
  sleep "$1" &
  SLEEP_PID=$!
  trap 'kill "$SLEEP_PID" 2>/dev/null; exit 0' 15
  wait "$SLEEP_PID"
  trap 'exit 0' 15
}
owns_session() {
  read -r owner 2>/dev/null < "$LOCK_DIR/pid" && [ "$owner" = "$$" ]
}
release_session() {
  owns_session || return 0
  rm -f "$LOCK_DIR/heartbeat" "$LOCK_DIR/game.pid" "$LOCK_DIR/pid" 2>/dev/null || true
  rmdir "$LOCK_DIR" 2>/dev/null || true
}
resume_platform() {
  "$KILLALL" -CONT awesome >/dev/null 2>&1 || true
  if [ "$1" = 1 ] && [ -n "$LIPC" ]; then
    "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.showMe();"}' >/dev/null 2>&1 || true
  fi
  if [ "$SCREENSAVER_BEFORE" = 0 ]; then
    "$LIPC" com.lab126.powerd preventScreenSaver 0 || echo 'WARN: Could not restore the screen saver.'
  fi
}
platinum_watchdog() {
  misses=0
  last_heartbeat=''
  while [ "$misses" -lt 5 ]; do
    watchdog_sleep 5
    owns_session || return 0
    if ! kill -0 "$$" 2>/dev/null; then
      echo 'Watchdog: KUAL launcher exited unexpectedly.'
      break
    fi
    if read -r heartbeat 2>/dev/null < "$LOCK_DIR/heartbeat" &&
       [ -n "$heartbeat" ] && [ "$heartbeat" != "$last_heartbeat" ]; then
      last_heartbeat="$heartbeat"
      misses=0
    else
      misses=$((misses + 1))
    fi
  done
  [ "$misses" -lt 5 ] || echo 'Watchdog: game heartbeat stopped.'
  child=''
  read -r child 2>/dev/null < "$LOCK_DIR/game.pid" || true
  case "$child" in
    ''|0|1|*[!0-9]*) kill -TERM "$$" 2>/dev/null || true ;;
    *)
      if kill -TERM "$child" 2>/dev/null; then
        watchdog_sleep 3
        owns_session || return 0
        kill -KILL "$child" 2>/dev/null || true
      fi ;;
  esac
  if kill -0 "$$" 2>/dev/null; then watchdog_sleep 1; fi
  owns_session || return 0
  resume_platform 1
  if ! kill -0 "$$" 2>/dev/null; then release_session; fi
}
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
  if [ -n "$WATCHDOG_PID" ]; then
    kill "$WATCHDOG_PID" >/dev/null 2>&1 || true
    wait "$WATCHDOG_PID" >/dev/null 2>&1 || true
  fi
  resume_platform "$PILLOW_HIDDEN"
  release_session
  echo 'UI restoration attempted.'
}
trap 'restore_ui' 0
trap 'exit 129' 1
trap 'exit 130' 2
trap 'exit 131' 3
trap 'exit 143' 15
if [ "$MODE" = platinum ]; then
  (trap '' 1 2 3; platinum_watchdog) </dev/null &
  WATCHDOG_PID=$!
  echo "Recovery watchdog PID: $WATCHDOG_PID (heartbeat monitored)"
  if [ "$SCREENSAVER_BEFORE" = 0 ]; then
    "$LIPC" com.lab126.powerd preventScreenSaver 1 || echo 'WARN: Could not postpone the screen saver.'
  fi
else
  (trap '' 1 2 3; watchdog_sleep "$RECOVERY_SECONDS"; resume_platform 1) </dev/null >/dev/null 2>&1 &
  WATCHDOG_PID=$!
  echo "Recovery watchdog PID: $WATCHDOG_PID ($RECOVERY_SECONDS-second fallback)"
fi
if [ -n "$LIPC" ]; then
  if "$LIPC" com.lab126.pillow interrogatePillow '{"pillowId": "default_status_bar", "function": "nativeBridge.hideMe();"}' >/dev/null 2>&1; then
    PILLOW_HIDDEN=1
    echo 'Pillow status bar requested hidden.'
  else
    echo 'WARN: Pillow hide command was rejected; continuing with WM guard.'
  fi
fi
if "$KILLALL" -STOP awesome >/dev/null 2>&1; then
  echo 'Awesome window manager paused; the session now owns the framebuffer.'
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
elif [ "$MODE" = platinum ]; then
  /bin/sh -c 'printf "%s\n" "$$" > "$1/game.pid" || exit 9; shift; exec "$@"' platinum "$LOCK_DIR" \
    "$EXE" --protected-session "$PLATINUM_ROOT/hero.gbc" "$PLATINUM_ROOT/hero.sav" "$LOCK_DIR/heartbeat" &
  GAME_PID=$!
  wait "$GAME_PID"
  RET=$?
  GAME_PID=''
elif [ "$MODE" = game ] || [ "$MODE" = standalone ]; then
  case "$MODE" in
    standalone)
      echo 'Starting the standalone opening (45-second maximum).'
      set -- --protected-session
      WATCHDOG_SECONDS=55 ;;
    *)
      echo 'Starting protected REAL NC1020 AUTO-GAME test: 30s; synthetic F11; MAIN/SAVE disabled; no state/Flash writes.'
      set -- --safe-game-test
      WATCHDOG_SECONDS=45 ;;
  esac
  "$EXE" "$@" &
  GAME_PID=$!
  # Independent recovery if the fullscreen child fails to exit.
  (
    trap '' 1 2 3
    watchdog_sleep "$WATCHDOG_SECONDS"
    kill -TERM "$GAME_PID" >/dev/null 2>&1 || true
    watchdog_sleep 3
    kill -KILL "$GAME_PID" >/dev/null 2>&1 || true
    resume_platform 1
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
printf 'Session process exit code: %s\n' "$RET"
exit "$RET"
