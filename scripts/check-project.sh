#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
sh -n scripts/bootstrap-kindle5-toolchain.sh scripts/build-armel.sh \
  scripts/package-kual.sh scripts/package-rom-free.sh scripts/package-device-check.sh \
  kual/extensions/golden_hero/launch.sh kual/extensions/golden_hero/display-test.sh \
  kual/extensions/golden_hero/diagnose.sh kual/extensions/GoldenHeroFullscreen/guarded-test.sh \
  kual/extensions/GoldenHeroFullscreen/test-fullscreen.sh kual/extensions/GoldenHeroFullscreen/test-touch.sh \
  kual/extensions/GoldenHeroFullscreen/test-controls.sh kual/extensions/GoldenHeroFullscreen/test-ui.sh \
  kual/device-check/diagnose.sh
python3 -m json.tool kual/extensions/golden_hero/menu.json >/dev/null
python3 -m json.tool kual/extensions/GoldenHeroFullscreen/menu.json >/dev/null
python3 -m json.tool kual/device-check/menu.json >/dev/null
python3 -m py_compile scripts/fetch_upstream.py
python3 tests/test_upstream_patch.py
python3 tests/test_storage_patch.py
mkdir -p build-tests
${CXX:-c++} -std=c++11 -Wall -Wextra -Ikindle tests/test_touch_state.cpp kindle/touch.cpp -o build-tests/test_touch_state
./build-tests/test_touch_state
${CXX:-c++} -std=c++11 -Wall -Wextra -Ikindle tests/test_mono_blit.cpp kindle/fb.cpp -o build-tests/test_mono_blit
./build-tests/test_mono_blit
./scripts/build-host-mock.sh
python3 tests/test_rom_requirements.py
./scripts/package-device-check.sh
printf '\nProject checks passed. Device check ZIP: %s\n' \
  "$(pwd)/Kindle-KPW1-DeviceCheck-v0.3.zip"
