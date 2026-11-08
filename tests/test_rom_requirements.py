#!/usr/bin/env python3
"""Host-only validation: --check-files must never load dummy ROM data."""
from pathlib import Path
import subprocess
import tempfile

binary = Path(__file__).resolve().parents[1] / 'build-host-mock/golden_hero'
if not binary.is_file():
    print('Host mock has not been built; skipping ROM CLI test')
else:
    with tempfile.TemporaryDirectory(prefix='golden_hero_rom_check_') as tmp:
        root = Path(tmp)
        def call(*args):
            return subprocess.run([str(binary), *args], cwd=root,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
        assert call('--check-files') == 3, 'Missing ROM files must fail'
        assert call('--display-test') == 10, 'Display test bypasses ROM requirement and fails only on missing host fb0'
        (root / 'roms').mkdir()
        (root / 'roms/obj_lu.bin').touch()
        (root / 'roms/nc1020.fls').touch()
        assert call('--check-files') == 3, 'Wrong sized ROM files must fail'
        with open(root / 'roms/obj_lu.bin', 'wb') as f:
            f.truncate(25165824)
        with open(root / 'roms/nc1020.fls', 'wb') as f:
            f.truncate(1048576)
        assert call('--check-files') == 0, 'Valid sizes must pass without initializing emulator'
    print('ROM size and ROM-free display-test argument checks passed.')
