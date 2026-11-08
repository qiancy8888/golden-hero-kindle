# Build the KPW1 ARM executable on GitHub Actions

The project includes `.github/workflows/build-kpw1.yml`. Once this source tree is pushed to a GitHub repository's `main` branch, the workflow can build the KPW1 executable in an Ubuntu runner.

The workflow:
1. Installs CMake, Python and zstd.
2. Downloads the KOReader `kindle5` cross-toolchain (the target for Kindle 4/Touch/PW1).
3. Fetches only the pinned NC1020 emulator source files; it does not download ROM/flash images.
4. Builds and verifies the ARM soft-float ELF and checks its interpreter.
5. Runs the host mock regression build.
6. Runs source integrity, save-patch and ROM/file validation regression tests.
7. Packages a ROM-free KUAL installation ZIP, including a 40-second framebuffer/touch test and no firmware images.
8. Publishes both the executable and the ROM-free install ZIP as separate GitHub Actions artifacts.

To run manually: GitHub repository -> Actions -> Build Golden Hero for Kindle PW1 -> Run workflow.

The resulting executable is still not a complete game installation. The user must separately supply legally usable `obj_lu.bin` and `nc1020.fls` files before packaging the KUAL installation ZIP.
