#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scripts.fetch_upstream import patch_safe_storage


def test(linebreak: bytes):
    source = linebreak.join([
        b'#include <stdlib.h>',
        b'void SaveNor(){',
        b'    FILE* file = fopen(nc1020_rom.norFlashPath.c_str(), "wb");',
        b'    fwrite(temp_buff, 1, NOR_SIZE, file);',
        b'}',
        b'void LoadStates(){',
        b'    fread(&nc1020_states, 1, sizeof(nc1020_states), file);',
        b'}',
        b'void SaveStates(){',
        b'    fwrite(&nc1020_states, 1, sizeof(nc1020_states), file);',
        b'}',
    ])
    result = patch_safe_storage(source)
    assert b'SaveBufferAtomically' in result
    assert result.count(b'rename(staged.c_str(), filename.c_str())') == 1
    assert b'const size_t read_size = fread(&saved' in result
    assert b'saved.version != VERSION' in result
    assert b'NC1020: state could not be safely saved' in result
    assert b'#include <unistd.h>' in result
    try:
        patch_safe_storage(result)
    except ValueError:
        pass
    else:
        raise AssertionError('Applying patch twice must be rejected')


if __name__ == '__main__':
    test(b'\r\n')
    test(b'\n')
    print('NC1020 atomic-save and state-load patch tests passed.')
