Runtime writable data directory.

At first launch the program copies roms/nc1020.fls to data/nc1020.fls.
The emulator writes the mutable flash and complete state here:
  nc1020.fls
  nc1020.sts

Back up this directory before replacing ROMs or emulator builds.

v0.6 uses temporary `.new` files and rename when saving NOR flash/state.
An interrupted save should leave the previous committed file intact, subject to filesystem behavior. Do not delete or overwrite game saves during operation. Back up data/ before updating versions.
