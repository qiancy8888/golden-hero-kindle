# Standalone research: direct launch of Golden Hero on KPW1

## Goal

Build a Kindle-native application shell that presents only the game screen and controls, with no visible NC1020 desktop/application launcher. Keep this work on the \`standalone-prototype\` branch until the boot path and device stability are demonstrated.

## Important technical boundary

The pinned upstream project (Wang-Yue/NC1020) is a WQX/NC1020 **emulator**, not the Golden Hero game's source project. Its README describes it as a simulator that can play 黄金英雄坛说. The repository's source files are \`main.cpp\`, \`nc1020.cpp\`, and \`nc1020.h\`; the separate binary images are \`obj_lu.bin\` (24 MiB) and \`nc1020.fls\` (1 MiB). The upstream source does not contain game C/C++ source files or a native ARM game target.

Therefore, we cannot truthfully "extract the game source from the emulator and recompile the game" based on this repository. To produce a true native Kindle game, the game itself would need to be independently reimplemented or ported from an authorized source. That is a much larger reverse-engineering project and may not reproduce original behavior.

The practical route is a **game-only Kindle frontend with the smallest necessary WQX runtime**:
- Retain the CPU interpreter, memory map, ROM/NOR access, keyboard matrix, timer/interrupt behavior and LCD buffer that the original WQX program requires.
- Remove SDL and the general-purpose NC1020 desktop UI.
- Replace the frontend with a Kindle framebuffer/touch UI, already partly present in \`kindle/main.cpp\`.
- Research and invoke the game's firmware/application launch path so the user never sees the system desktop. This is still a WQX runtime internally, not a native recompile of the game.

## What source inspection establishes

At pinned upstream commit \`95777cad9d82a29dcb4e78445ee745bf4d2c2286\`:

1. \`wqx::Initialize(WqxRom)\` maps the ROM volumes and loads the 24 MiB \`obj_lu.bin\` system image.
2. \`wqx::LoadNC1020()\` loads the 1 MiB NOR image and then attempts to restore a serialized NC1020 CPU/RAM state.
3. If no valid saved state is restored, \`ResetStates()\` initializes the CPU registers and sets \`PC = PeekW(0xFFFC)\`, the reset vector. That is a system reset/boot path, not a Golden Hero game entry point.
4. \`wqx::RunTimeSlice()\` executes the emulated CPU; \`SetKey()\` writes the NC1020 keyboard matrix; \`CopyLcdBuffer()\` returns the 160x80 LCD image.
5. The upstream GameShell frontend maps Space to key \`0x0E\` (F11, described as the game menu), U/I/J/K to F1/F4/Enter/Escape, Y/0 to Shift-X/Shift-Y, and Backspace to quit the host application. F11 is a menu/input event, not evidence of a direct game executable entry point.

## Why v0.9.5 did not directly boot the game

The KPW1 build currently:
- Initializes the ROM and NOR.
- Runs the emulated firmware for six seconds.
- Injects F11 (0x0E) for 300 ms.
- Waits another 750 ms and begins displaying the LCD.

The real device still showed an application/launcher screen and then timed out. This means the code sent a key, but we have not yet identified the next menu action or a lower-level game entry routine. More delay alone is not a supported fix.

## Instrumentation added on this branch

The source-fetch script adds a read-only \`wqx::DebugSnapshot\` API to the pinned upstream core. Around startup and F11 it records:
- PC, reset vector, A/X/Y/SP/status registers and opcode bytes at PC;
- NOR bank, ROM volume selector, memory mapping selector and selected I/O registers;
- emulated cycle count, LCD buffer address, sleep state and keyboard matrix;
- LCD framebuffer hash.

This instrumentation does not change the ROM, NOR Flash, CPU execution, or key mapping. It does not dump ROM/NOR contents and does not add proprietary image files to the repository.

## Next investigation sequence

1. Build this branch and run a protected 30-second diagnostic on the KPW1 with the user's own locally stored images.
2. Compare snapshots at initial load, after firmware settling, immediately before/while/after F11, and during the application screen.
3. Capture the actual 160x80 LCD frames (or user photos) at each transition.
4. Identify the application launcher screen and determine which additional key/selection sequence starts Golden Hero.
5. Inspect the relevant firmware/flash structures locally, without uploading or publishing the user's ROM/NOR files. Identify whether the game code resides in NOR, a ROM volume, or a loaded RAM region, and whether the launcher calls an OS service.
6. Only after a repeatable direct-launch path is understood, add a game-only startup mode and verify it on hardware.

## Safety and distribution

- This branch is research-only. It is not a replacement installer for the user's current KPW1 build.
- ROM/NOR images and mutable saves remain outside Git and outside CI artifacts.
- Never overwrite the user's original \`roms/nc1020.fls\`; preserve a separate writable copy for experiments.
- Do not write guessed CPU addresses or jump to a guessed PC. A wrong direct jump can corrupt state or cause a device crash.
- Do not claim a native game recompilation unless the game's own source code is actually available and its rights permit that work.
