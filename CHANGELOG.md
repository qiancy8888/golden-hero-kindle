# Changelog

## v0.9.6 (2026-11-08) — enlarged LCD and simplified touch controls
- Remove the redundant on-screen X (Shift-X, 0x31) and Y (Shift-Y, 0x25) controls; retain FLY, MENU, OK, BACK, PGUP, PGDN, SAVE and EXIT.
- Replace integer 4x rendering with fractional nearest-neighbour scaling. On the 758px-wide KPW1 display, the 160x80 NC1020 LCD now uses 736x368 pixels instead of 640x320 (about 15% larger in each dimension).
- Reflow the remaining touch controls below the enlarged LCD; retain the same key mappings for all remaining controls.
- Add host regression coverage for fractional monochrome scaling. The automatic GAME key sequence is intentionally unchanged pending confirmation of the exact post-F11 screen.


## v0.9.5 (2026-11-08) — auto-GAME timing and X-key isolation
- User confirmed the on-screen X control (not Kindle hardware) precedes the Kindle OS reboot.
- Upstream NC1020 GameShell mapping identifies key 0x31 as Shift-X, not emulator exit or power. Temporarily disable this one key in the diagnostic build to isolate the reboot; restore it after safe testing.
- Increase NC1020 boot settling from 1.8s to 6.0s before synthetic F11/GAME, increase press duration from 125ms to 300ms, then allow 750ms for the menu transition.
- Log actual caught signal number separately from user-requested EXIT, to distinguish external process termination from the on-screen EXIT control.
- Keep the 30s no-save mode, MAIN/SAVE restrictions, single-instance wrapper and framebuffer restoration unchanged.
- This is a diagnostic hypothesis, not proof that boot timing or X key caused the reported reboot.


## v0.9.4 (2026-11-08) — Direct-to-game boot
- Let NC1020 firmware run for 1.8s, synthesize upstream F11/GAME (0x0E) for 125ms, then settle for 250ms.
- Perform automatic F11 before the first LCD copy so the NC1020 home screen is not intentionally rendered.
- Remove GAME and MAIN from touch controls.
- Retain the 30s no-save diagnostic, disabled MAIN/SAVE, single-instance fullscreen guard and recovery watchdog.
- Add source regression checks for automatic F11 and removed touch buttons.
- Physical KPW1 confirmation is still required; adjust boot timing if needed.


## v0.9.3 (2026-11-08) — KPW1 reboot safety hotfix
- Immediately disable all unprotected persistent game launches after the user reported a genuine Kindle system reboot on MAIN.
- Route the KUAL game launcher through the proven protected fullscreen wrapper and a single-owner /tmp lock.
- Add a strictly 30-second safe game session: start fresh from user-supplied ROM/NOR, do not load any existing saved state or write any state/Flash on SAVE/EXIT/timeout.
- Disable MAIN and SAVE while diagnosing the reboot; GAME, D-pad and OK remain available.
- Reduce the real emulator LCD refresh rate from ~5.5 updates/s to at most ~0.9/s.
- Restore the pre-test framebuffer as verified in v0.8.1; resume Awesome and Pillow after exit.
- Add a child process watchdog (TERM at 45 seconds, KILL 3 seconds later) alongside the existing independent Awesome/Pillow restoration fallback at 65 seconds.
- Previous logs show overlapping start lines and duplicate key events, suggesting concurrent emulator processes; this is not yet confirmed as the cause of the system reboot.
- Investigate persistence only after a safe session is successfully verified on physical hardware.


## v0.9.2 (2026-11-08)
- Move NC1020 ROM and initial NOR Flash lookup to `GOLDEN_HERO_PRIVATE_DIR/roms`; KUAL sets this to `/mnt/us/documents/GoldenHeroPrivate`.
- Move mutable NOR Flash and emulator state to the private sibling `data/` directory, preserving the initial image as read-only input.
- Make the KUAL launcher reject missing or wrong-sized private images and never fall back to files inside the app directory.
- Make both packaging entry points produce ROM-free installers; add private-folder setup instructions and regression tests for external-path loading.
- CI continues to fetch pinned emulator source only; it never fetches or packages ROM/firmware binaries.

## v0.9.1 (2026-11-08)
- Confirm real KPW1 UI viewport, 160x80 upscaled 4x, and touch layout from on-device photo.
- Add a separate original ROM-free mini town walking test: D-pad moves the player, held directions repeat, buildings and an NPC block movement, OK talks to NPC, MENU and BACK open/close overlays.
- Implement dirty-rectangle LCD refresh for changed bytes and reuse the same refresh path in the eventual real NC1020 game loop.
- Keep the v0.8.1 original-framebuffer backup, EXIT-on-finger-release, and KUAL recovery watchdog.
- Add host tests for movement, collision, key repeat and menu; still not the actual copyrighted game, which requires authorized images.


## v0.9.0 (2026-11-08)
- Add a ROM-free KUAL UI preview that renders a representative 160x80 monochrome LCD screen in the real game viewport alongside the same touch controls.
- Optimize 1bpp-to-8bpp LCD expansion with a row-buffer blitter instead of thousands of per-pixel framebuffer rectangle calls.
- Add host regression tests for bit order, integer scaling, framebuffer offsets and grayscale inversion.
- Reuse the previously hardware-verified fullscreen recovery guard; the preview restores the original framebuffer before returning to Kindle UI.
- This is a screen/layout validation milestone, not proof of game boot or gameplay; legally usable NC1020 images are still required for that.

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
