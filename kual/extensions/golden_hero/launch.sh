#!/bin/sh
# Safe KPW1 game launcher. Previous unguarded entry caused a system reboot
# on the user's device. Never run ./golden_hero directly from KUAL.
cd /mnt/us/extensions/GoldenHeroFullscreen || exit 2
exec /bin/sh ./guarded-test.sh game
