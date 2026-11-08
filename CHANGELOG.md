# Changelog

## v0.8.1 (2026-11-08)
- Fix the KPW1 controls-test EXIT path: it now waits for finger release, restores the exact pre-test framebuffer snapshot, and sends a full-screen EPDC refresh before the KUAL wrapper resumes Awesome and restores Pillow.
- Restore the framebuffer on timeout and handled SIGINT/SIGTERM as well as EXIT.
- Log snapshot size, exit reason, and framebuffer restore result for diagnosis.

## v0.8 (2026-11-08)
- Use the confirmed KPW1 cyttsp touch coordinates directly (758×1024, no axis swap or flips).
- Add SYN_REPORT-based touch state transitions: press, move, release, and mapped coordinates.
- Hold the corresponding NC1020 SetKey state for the entire finger contact; release it on lift or when dragging into another zone. This matches the upstream SDL key-down/key-up model.
- Add a ROM-free `--controls-test` mode using the exact same virtual-button hitboxes as the game UI; tap/hold/drag feedback is shown and logged.
- Add a protected KUAL fullscreen test entry reusing the proven v0.7 65-second watchdog, Pillow hide/show bridge, and Awesome STOP/CONT handoff. It does not stop Xorg/framework or modify WordScreen.
- Add host regression tests for touch press/move/release packet parsing and ensure the controls test bypasses ROM validation.
- The game itself still requires user-supplied legally usable NC1020 images; game input and save behavior remain unverified on hardware until those images are available.

## v0.3 — KPW1 device check and toolchain targeting

- Target the `kindle5` koxtoolchain family (Kindle 4 / Touch / PW1), not PW2/PW3 targets.
- Add pinned, integrity-checked source-only retrieval for the NC1020 emulator core.
- Add a verified toolchain bootstrap script and stricter ELF checks.
- Improve touchscreen event-device selection to avoid confusing accelerometers with touch panels.
- Validate framebuffer virtual dimensions, stride, offsets, and mapped memory bounds.
- Add a standalone KUAL device-check extension that reports the actual KPW1 firmware, framebuffer, touch devices, and runtime libraries without modifying settings.
- The game launcher temporarily inhibits the screen saver while running and restores the previous property on exit; it does not stop `powerd` or modify WordScreen.


## v0.4 (2026-11-07)
- Applied the user's actual KPW1 diagnostic profile.
- Corrected device glibc documentation to 2.12.1.
- Confirmed the official KOReader `kindle5` toolchain target for PW1.
- Added a read-only LuaJIT FFI probe for framebuffer geometry and touch ABS ranges.
- Removed unsupported `command -v` checks from the legacy-shell diagnostic.
- Still source/probe only; no playable ARM binary or game ROM is included.


## v0.7 (2026-11-08)

- Investigate KPW1 user photo: Kindle native library visible, TOP ghosting; v0.6 logs alone did not establish full-screen render success.
- Increase no-ROM screen test timeout from 40 to 75 seconds for easier photo capture during test.
- Log actual exit reason/elapsed time and count raw touchscreen events (EV_ABS, EV_KEY, EV_SYN), limited raw trace to 160 events.
- Sample selected framebuffer pixels every 2.5 seconds to detect potential overwrites by other processes such as Xorg.
- Continue to restore previous framebuffer on normal exit; do not stop Xorg or powerd or modify WordScreen.

## v0.6 (2026-11-08)
- Added a ROM-free `--display-test` mode that opens the real framebuffer, displays orientation and grayscale calibration, prints touch coordinates, and exits after 40 seconds or by tapping EXIT.
- Display test copies and restores the visible framebuffer on normal exit, and does not access NC1020 ROM or save files.
- Added a standalone KUAL display test entry and a ROM-free USB-root installation ZIP built by GitHub Actions.
- Added tests for ROM file size validation and ROM-free CLI bypass.
- Replaced pinned upstream direct-overwrite save code with temporary writes + rename, and validated saved-state size/version before restoring.
- Added storage patch tests. KPW1 device drawing, touch handling, and game operation still require real-device verification.

## v0.5 (2026-11-07)
- Integrated the actual framebuffer/touch probe values from the user's KPW1.
- Confirmed direct touch coordinate mapping (0..758, 0..1024), with no default axis swap/flip.
- Added framebuffer profile diagnostics and MXCFB ABI compile-time size assertions.
- Expanded the touchscreen control layout to expose the upstream key map (FLY, MENU, OK, BACK, MAIN, GAME, X, Y, PGUP, PGDN, SAVE, EXIT).
- Added missing C/H glyphs used by the control labels.
- Made the KUAL launcher find LIPC commands without `command -v`; only changes preventScreenSaver if both getter/setter exist and the original value is known.
- Host mock C++ build and project shell/JSON checks pass.
- No KPW1 ARM binary, ROM/flash image, or real-device draw test is included yet.
- Pinned NC1020 source inspection found a duplicated `rom_volume1` assignment in `Initialize`; fetch script now applies a verified local correction to initialize `rom_volume2`.
