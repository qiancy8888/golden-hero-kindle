#!/bin/sh
cd /mnt/us/extensions/GoldenHeroStandalone || exit 2
exec /bin/sh ./guarded-test.sh standalone
