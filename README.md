# Golden Hero on Kindle Paperwhite 1 (KPW1)

An experimental port of the NC1020 emulator core to the first-generation Kindle Paperwhite. The goal is to run **Golden Hero Tan Shuo (黄金英雄坛说)** on the KPW1 e-ink display with a KUAL launcher and touchscreen controls.

**Status: development / not playable yet.** This repository currently contains the porting source, device profile, build scripts, and a GitHub Actions workflow. It does not contain a ready-to-install Kindle executable or game images.

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

The emulator expects these files, which are deliberately not included in this repository:

- `roms/obj_lu.bin` (24 MiB)
- `roms/nc1020.fls` (1 MiB)

The NC1020 source-code license does not automatically grant redistribution rights for ROM or firmware images. Only use image files that you are legally entitled to use.

## Current limitations

- No ready-to-install KUAL game package is published.
- No KPW1 ARM executable has yet been confirmed working on real hardware.
- Framebuffer drawing, touch hit areas, save persistence, and long-running behavior still need on-device validation.
- The included host mock is for build/regression checks only; it is not the game emulator.

## Project structure

- `kindle/` — framebuffer, e-ink refresh, touch input, and application loop
- `kual/` — KUAL launcher and device diagnostic extensions
- `scripts/` — upstream source retrieval, toolchain bootstrap, build, and packaging
- `docs/` — KPW1 device profile, touch controls, and build notes
- `tests/` — host mock and source-patch regression test

## Credits and licensing

The emulator core is based on [Wang-Yue/NC1020](https://github.com/Wang-Yue/NC1020). Its upstream license is retained in the fetched `upstream/LICENSE` file at build time. This repository does not bundle upstream game/firmware images. See the repository `LICENSE` for the original project files and respect the upstream license for third-party code.
