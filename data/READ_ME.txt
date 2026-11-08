Runtime writable data is stored outside the app directory on Kindle:
  /mnt/us/documents/GoldenHeroPrivate/data/nc1020.fls
  /mnt/us/documents/GoldenHeroPrivate/data/nc1020.sts

At first launch, the emulator copies the initial image from
GoldenHeroPrivate/roms/nc1020.fls into this data directory. It then writes
the mutable flash and complete emulator state here, leaving the initial
image unchanged.

Back up GoldenHeroPrivate/data before replacing images or emulator builds.
The core uses temporary .new files and rename for NOR/state saves; an
interrupted save should leave the previous committed file intact, subject
to filesystem behavior.
