# KPW1 private ROM and save-data setup

This project separates the emulator application from user-provided game data.

## Kindle folder layout

After installing the ROM-free KUAL package, create this folder on the Kindle USB storage:

`documents/GoldenHeroPrivate/roms/`

Place only the image files you are entitled to use there:

| File | Required size |
|---|---:|
| `obj_lu.bin` | 25,165,824 bytes (24 MiB) |
| `nc1020.fls` | 1,048,576 bytes (1 MiB) |

The complete path on the device is:

- `/mnt/us/documents/GoldenHeroPrivate/roms/obj_lu.bin`
- `/mnt/us/documents/GoldenHeroPrivate/roms/nc1020.fls`

The launcher creates the sibling `data/` folder automatically. The first game launch copies the initial `roms/nc1020.fls` to `data/nc1020.fls`. The emulator writes its mutable NOR Flash and state to:

- `/mnt/us/documents/GoldenHeroPrivate/data/nc1020.fls`
- `/mnt/us/documents/GoldenHeroPrivate/data/nc1020.sts`

The original files under `roms/` are input images; the game should modify only files under `data/`.

## Install and start

1. Install the ROM-free ZIP at the Kindle USB root. It contains the program, KUAL launcher and this guide, but no game/firmware images.
2. Create `documents/GoldenHeroPrivate/roms/`.
3. Copy your separately obtained, personally authorized images into that folder. This project does not provide download links or retrieve ROM files.
4. Safely eject the Kindle.
5. Open KUAL → Golden Hero → **黄金英雄坛说（读取个人私有镜像）**.

If an image is missing or has the wrong exact size, the launcher stops before starting the emulator and records the expected path and size in `/mnt/us/golden_hero/logs/launcher.log`.

## Updates and backups

- Reinstalling/updating `/mnt/us/golden_hero/` does not replace `GoldenHeroPrivate/`.
- Back up the entire `documents/GoldenHeroPrivate/data/` folder regularly; it contains the mutable Flash and emulator save state.
- Do not replace ROM/Flash inputs or copy save files while the game is running.
- Never add private images or saves to the Git repository, issue attachments, public build artifacts, or public releases.
- The KUAL launcher explicitly sets `GOLDEN_HERO_PRIVATE_DIR`; the executable does not fall back to a ROM folder inside the application directory.

## Developer override

For desktop testing, the executable defaults to `./roms` and `./data` relative to its current working directory. Set `GOLDEN_HERO_PRIVATE_DIR` to another directory to test an external private data root. The same exact file-size checks apply.

This folder separation is a technical data-handling measure, not a statement that any particular ROM image is licensed. The upstream NC1020 source-code license does not automatically grant rights to its separate game/firmware images. Use only files you are legally entitled to use.
