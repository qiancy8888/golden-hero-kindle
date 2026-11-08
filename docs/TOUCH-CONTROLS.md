# KPW1 touch controls (v0.5)

The original NC1020 screen is 160×80 monochrome pixels. On the PW1's measured 758×1024 portrait panel, the game image is centered near the top and enlarged with an integer scale of 4 (640×320). The controls occupy the lower half of the screen.

## Touch mapping

| Button | NC1020 key | Function from upstream GameShell mapping |
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
| SAVE | emulator action | Save NC1020 NOR flash and emulator state |
| EXIT | emulator action | Exit and save on shutdown |

The four directional touch zones form a non-overlapping cross. The remaining 12 actions use two columns of large touch targets. All hitboxes are within the measured 758×1024 visible area and have no overlaps.

## Refresh behavior

- Static UI: one full-screen update at startup.
- Game LCD: partial update only when the 160×80 source buffer changes.
- Periodic full refresh: every 180 seconds to reduce accumulated ghosting.
- Touch actions: a short key-down interval is sent to the emulator, then released.

The first real-device display test must still verify the visual orientation, grayscale polarity and touch accuracy before this is treated as a playable release.
