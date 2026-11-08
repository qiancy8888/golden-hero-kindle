#!/bin/sh
# Safe, ROM-free 40-second framebuffer and touch smoke test.
cd /mnt/us/golden_hero || exit 1
mkdir -p logs || exit 1
if [ ! -x ./golden_hero ]; then
  echo "Missing ARM game executable; copy the entire ROM-free install ZIP to the Kindle USB root." >> logs/display-test.log
  exit 2
fi
printf '\n=== Golden Hero KPW1 display test ===\n' >> logs/display-test.log
./golden_hero --display-test >> logs/display-test.log 2>&1
code=$?
printf 'Test exit code: %s\n' "$code" >> logs/display-test.log
exit "$code"
