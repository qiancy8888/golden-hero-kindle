# KPW1 touch controls (v0.8)

The physical KPW1 touch panel was verified by the user: visible touch markers landed at the finger locations. Use the measured 758×1024 coordinate space directly (X 0–758, Y 0–1024), with no axis swap or flip.

## NC1020 key map

| Button | NC1020 key | Upstream meaning |
|---|---:|---|
| UP / DOWN / LEFT / RIGHT | 0x1A / 0x1B / 0x3F / 0x1F | Direction keys |
| FLY | 0x10 | F1 |
| MENU | 0x13 | F4 |
| OK | 0x1D | Enter |
| BACK | 0x3B | Escape |
| MAIN | 0x08 | F10 / main screen |
| GAME | 0x0E | F11 / game menu |
| X | 0x31 | Shift-X |
| Y | 0x25 | Shift-Y |
| PGUP | 0x37 | Page Up |
| PGDN | 0x1E | Page Down |
| SAVE | emulator action | Save NC1020 NOR flash and state |
| EXIT | emulator action | Exit and save on shutdown |

The key IDs above are taken from the upstream NC1020 SDL keyboard switch, not inferred from labels.

## v0.8 input semantics

- Linux input events are consumed in SYN_REPORT packets.
- A finger-down event presses the selected NC1020 key.
- The key remains down for the entire contact, allowing the emulator to observe a held key across consecutive RunTimeSlice calls.
- A finger-up event releases the key.
- Dragging to another button releases the old key before pressing the new one.
- SAVE fires once on the initial down event; EXIT exits only after the finger is released.
- The controls test snapshots the visible framebuffer before drawing, restores the original pixels on EXIT/timeout/signal, and sends a full-screen EPDC refresh before returning to the KUAL wrapper.
- The ROM-free controls test uses the same layout/hitboxes, shows a gray pressed-button state, logs DOWN/DRAG/UP and key IDs, and auto-exits after 45 seconds.

## Protected fullscreen test

KUAL launches the test through the proven v0.7 protected fullscreen guard: it checks Kindle FW 5.6.1.1 and the mxc_epdc_fb driver, starts a 65-second recovery watchdog, asks Pillow to hide the native status bar, pauses only Awesome (never Xorg or framework), then restores Awesome and the status bar on exit. WordScreen files and power settings are not changed.

The no-ROM test validates touch hitboxes and input transitions only. Actual character movement and gameplay still require user-provided, legally usable NC1020 images and a separate in-game test.
