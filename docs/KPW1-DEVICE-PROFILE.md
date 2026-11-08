# KPW1 device profile

Source: user's two read-only diagnostic reports. No serial number or board ID is retained.

| Field | Measured value |
|---|---|
| Device | Kindle Paperwhite 1 |
| Firmware | 5.6.1.1 |
| Kernel | 2.6.31-rt11-lab126 |
| Architecture | 32-bit ARMv7 |
| CPU core | Cortex-A8 family (CPUID part 0xC08) |
| Board family | MX50 YOSHIME |
| RAM | 256 MiB |
| Framebuffer driver | mxc_epdc_fb |
| Framebuffer device | /dev/fb0 |
| Physical xres/yres | 758 x 1024 |
| Virtual xres/yres | 768 x 6144 |
| xoffset/yoffset | 0 / 0 |
| Pixel depth | 8 bpp |
| Grayscale mode | 1 (8-bit grayscale) |
| Rotation value | 3 |
| Line length | 768 bytes |
| Framebuffer memory | 4,718,592 bytes (4.5 MiB) |
| Touch controller | cyttsp |
| Input node | /dev/input/event0 |
| ABS_MT_POSITION_X | min 0, max 758 |
| ABS_MT_POSITION_Y | min 0, max 1024 |
| Dynamic loader | /lib/ld-linux.so.3 |
| glibc | 2.12.1 |
| libstdc++ | 6.0.14 |

## Coordinate decision

The touch axis maxima match the framebuffer's reported visible dimensions. The port therefore starts with direct X/Y normalization and no axis swap or flip. The `rotate=3` value is logged but not applied as an additional software rotation because the reported visible xres/yres are already portrait and match the touch axes. The first actual draw test must still confirm the visual orientation and hit areas.

## Memory layout

The reported framebuffer memory length is exactly:

`768 * 6144 = 4,718,592 bytes`

Each visible row has a 768-byte stride while only 758 pixels are visible. Pixel addressing must use `y * stride + x`, not `y * 758 + x`. The port's framebuffer adapter uses the kernel-reported `line_length` and `smem_len`, and maps the full reported memory length.

## Refresh ABI

The adapter uses the PW1 MXCFB update structure. Compile-time checks require a 16-byte rectangle and 72-byte update structure, preventing accidental ABI padding changes on the 32-bit ARM target.
