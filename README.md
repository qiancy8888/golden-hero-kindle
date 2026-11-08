# Golden Hero on Kindle Paperwhite 1 (KPW1)

An experimental port of the NC1020 emulator core to the first-generation Kindle Paperwhite. The goal is to run **Golden Hero Tan Shuo (黄金英雄坛说)** on the KPW1 e-ink display with a KUAL launcher and touchscreen controls.

**Status: v0.9.5 direct-to-game boot (physical KPW1 verification pending).** KPW1 display and touch coordinates have been confirmed on hardware. v0.9.1 adds a ROM-free miniature town test; v0.9.2 moves user-provided NC1020 images and mutable saves completely outside the app installation directory. The KUAL launcher sets `GOLDEN_HERO_PRIVATE_DIR` to `/mnt/us/documents/GoldenHeroPrivate`. After the report of an actual KPW1 OS reboot, unprotected persistent launches remain disabled. The protected 30s diagnostic now automatically sends NC1020 F11/GAME after a 1.8s firmware startup delay, before the first LCD frame is drawn; the GAME and MAIN touch buttons are removed. It does not load or write persistent save data. GitHub Actions and install ZIPs never fetch, bundle, or distribute game images.

[简体中文说明](README_中文.md) · [Changelog](CHANGELOG.md)

## Target device

- Kindle Paperwhite 1 (KPW1), firmware 5.6.1.1
- Screen: 758 × 1024, 8-bit grayscale framebuffer, 768-byte line stride
- Touch axes: X 0–758, Y 0–1024
- Target toolchain: KOReader `kindle5` (ARM EABI soft-float)

The device profile is based on read-only diagnostics collected from a physical KPW1. The first on-device rendering test is still required to confirm display orientation and refresh behavior.

## Build

The `Build Golden Hero for Kindle PW1` GitHub Actions workflow can be run manually from **Actions**, or triggered by relevant source changes on `main`. It installs the KOReader `kindle5` cross-toolchain, fetches a pinned copy of the NC1020 emulator source (source files only), applies a documented local source fix, builds the ARM executable, and uploads it as a workflow artifact.

For a local Linux/WSL2 build:

```sh
./scripts/bootstrap-kindle5-toolchain.sh
./scripts/build-armel.sh
```

The expected executable is `build-armel/golden_hero`. A successful cross-build is not proof that the program has been tested on the device.

## Game images are not included

The emulator reads these files from the Kindle user's private data area, not from the app directory:

- `/mnt/us/documents/GoldenHeroPrivate/roms/obj_lu.bin` (25,165,824 bytes)
- `/mnt/us/documents/GoldenHeroPrivate/roms/nc1020.fls` (1,048,576 bytes)

Mutable NOR Flash and state are kept in the sibling `data/` folder. The original `roms/nc1020.fls` is copied on first launch and never used as the writable save file. The repository and installer deliberately do not fetch, bundle, or distribute these images. The NC1020 source-code license does not automatically license the binary images. Only use images you are legally entitled to use.

See [PRIVATE-ROM-SETUP.md](docs/PRIVATE-ROM-SETUP.md) for the on-device folder setup.

## Current limitations

- CI generates a ROM-free KUAL install ZIP with 45-second touch-control and LCD-layout previews, each using a 65-second fullscreen recovery watchdog.
- The standard `scripts/package-kual.sh` entry point also creates a ROM-free installer; game images are never copied into it.
- KPW1 display, grayscale refresh, and touch coordinate accuracy have been confirmed on the user's physical device.
- v0.8 implements key-down/key-up hold behavior, but the no-ROM test validates only touch hitboxes and event state; actual in-game movement still awaits legally usable game images.
- Save persistence and long-running game behavior remain to be tested.
- KUAL always uses the external `GoldenHeroPrivate` folder; it does not fall back to ROMs placed beside the executable.
- The included host mock is for build/regression checks only; it is not the game emulator.

## Project structure

- `kindle/` — framebuffer, e-ink refresh, touch input, ROM-free display test, and application loop
- `kual/` — KUAL launcher and device diagnostic extensions
- `scripts/` — upstream source retrieval, toolchain bootstrap, build, and packaging
- `docs/` — KPW1 device profile, touch controls, and build notes
- `tests/` — host mock and source-patch regression test

## Credits and licensing

The emulator core is based on [Wang-Yue/NC1020](https://github.com/Wang-Yue/NC1020). Its upstream license is retained in the fetched `upstream/LICENSE` file at build time. This repository does not bundle upstream game/firmware images. See the repository `LICENSE` for the original project files and respect the upstream license for third-party code.
