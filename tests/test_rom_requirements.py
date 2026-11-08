#!/usr/bin/env python3
"""Validate default and external private ROM paths without initializing the core."""
from pathlib import Path
import os
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
source = (project / 'kindle/main.cpp').read_text(encoding='utf-8')
assert 'AUTO-GAME: F11 key-down (0x0E)' in source, 'Automatic F11 must remain enabled'
actions = source.split('const struct {int key;const char* label;} actions[]={', 1)[1].split('};', 1)[0]
assert '"GAME"' not in actions and '"MAIN"' not in actions, 'GAME and MAIN must not be touch controls'
assert '{-3,"X (disabled)"}' in actions, 'X must be isolated during reboot diagnosis'
assert 'for(int i=0;i<240;++i) wqx::RunTimeSlice(25,false);' in source, '6-second boot delay must remain'
assert 'caught_signal' in source, 'Actual external signal must be logged'
assert 'b.key==-2 || b.key==-3 || b.key==0x08' in source, 'X/MAIN/SAVE must remain disabled'
assert 'SAFE GAME TEST: X drag ignored' in source, 'Dragging onto X must not inject Shift-X'
binary = project / 'build-host-mock/golden_hero'
if not binary.is_file():
    print('Host mock has not been built; skipping ROM CLI test')
else:
    with tempfile.TemporaryDirectory(prefix='golden_hero_rom_check_') as tmp:
        root = Path(tmp) / 'cwd'
        root.mkdir()
        private = Path(tmp) / 'private'
        def call(*args, private_dir=None):
            env = os.environ.copy()
            env.pop('GOLDEN_HERO_PRIVATE_DIR', None)
            if private_dir is not None:
                env['GOLDEN_HERO_PRIVATE_DIR'] = str(private_dir)
            return subprocess.run([str(binary), *args], cwd=root, env=env,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode

        assert call('--check-files') == 3, 'Missing default ROM files must fail'
        assert call() == 20, 'Unprotected game launch must fail closed'
        assert call('--safe-game-test') == 3, 'Missing image files must fail in safe game mode'
        for flag in ('--display-test', '--controls-test', '--ui-test', '--demo-test'):
            assert call(flag) == 10, f'{flag} must bypass ROM requirement and fail only on missing host fb0'

        # Default developer mode remains relative to the current directory.
        (root / 'roms').mkdir()
        (root / 'roms/obj_lu.bin').touch()
        (root / 'roms/nc1020.fls').touch()
        assert call('--check-files') == 3, 'Wrong-sized default ROM files must fail'
        with open(root / 'roms/obj_lu.bin', 'wb') as f:
            f.truncate(25165824)
        with open(root / 'roms/nc1020.fls', 'wb') as f:
            f.truncate(1048576)
        assert call('--check-files') == 0, 'Valid default sizes must pass'

        # Explicit private root overrides the app/CWD completely.
        private_roms = private / 'roms'
        private_roms.mkdir(parents=True)
        with open(private_roms / 'obj_lu.bin', 'wb') as f:
            f.truncate(25165824)
        with open(private_roms / 'nc1020.fls', 'wb') as f:
            f.truncate(1048576)
        assert call('--check-files', private_dir=private) == 0, 'External private ROM path must pass'
        (private_roms / 'nc1020.fls').unlink()
        assert call('--check-files', private_dir=private) == 3, 'Must not fall back to app-local ROMs'
    print('ROM path tests passed: default, external private root, strict no-fallback, exact sizes.')
