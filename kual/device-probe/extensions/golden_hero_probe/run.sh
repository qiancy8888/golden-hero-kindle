#!/bin/sh
# Read-only ioctl probe. Does not draw to the display or change power settings.
BASE=/mnt/us/golden_hero_probe
OUT="$BASE/logs/fb-input-probe.txt"
mkdir -p "$BASE/logs" 2>/dev/null || exit 1
{
  echo "KPW1 FRAMEBUFFER / TOUCH IOCTL PROBE v0.4"
  echo "Generated: `date 2>/dev/null`"
  echo "This probe only reads framebuffer/input ioctls."
} > "$OUT"

LUAJIT=""
for p in /mnt/us/koreader/luajit \
         /mnt/us/extensions/koreader/luajit \
         /mnt/us/extensions/koreader/bin/luajit \
         /mnt/us/koreader/bin/luajit; do
  if [ -x "$p" ]; then LUAJIT="$p"; break; fi
done

if [ -z "$LUAJIT" ]; then
  for root in /mnt/us/koreader /mnt/us/extensions/koreader /mnt/us/extensions/KOReader; do
    [ -d "$root" ] || continue
    candidate=`find "$root" -type f -name luajit 2>/dev/null | head -n 1`
    if [ -n "$candidate" ] && [ -x "$candidate" ]; then
      LUAJIT="$candidate"
      break
    fi
  done
fi

if [ -z "$LUAJIT" ]; then
  {
    echo
    echo "ERROR: KOReader LuaJIT executable was not found."
    echo "Expected a luajit binary under /mnt/us/koreader or /mnt/us/extensions/koreader."
    echo "No files or device settings were changed."
  } >> "$OUT"
  exit 2
fi

LUA_HOME=`dirname "$LUAJIT"`
export LD_LIBRARY_PATH="$LUA_HOME/libs:$LUA_HOME/lib:$LUA_HOME/../libs:$LUA_HOME:$LD_LIBRARY_PATH"
cd "$BASE" || exit 1
"$LUAJIT" "$BASE/probe.lua" >> "$OUT" 2>&1
RC=$?
echo "LuaJIT exit code: $RC" >> "$OUT"
echo "Report: $OUT"
exit "$RC"
