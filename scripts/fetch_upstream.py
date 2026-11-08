#!/usr/bin/env python3
"""Fetch only the BSD-licensed NC1020 emulator source at a pinned commit.

The upstream repository also contains game/firmware images. This script never
requests or downloads those binary files.
"""
from hashlib import sha1
from pathlib import Path
import re
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent
COMMIT = "95777cad9d82a29dcb4e78445ee745bf4d2c2286"
BASE = f"https://raw.githubusercontent.com/Wang-Yue/NC1020/{COMMIT}"
FILES = {
    "nc1020.cpp": "f13da3cf6971de06a266c418acdad7fd6c70fd51",
    "nc1020.h": "b0821f5ac625c01ac624dca3a807026e5866b4d1",
    "LICENSE": "4729d284609ce9b4a9840fa1ebe09802926d55f4",
    "README.md": "5aa4afb9121117ded30f4f77e49e4b6663d7d638",
}


def patch_nc1020_source(data: bytes) -> bytes:
    """Correct a duplicated ROM volume pointer in the pinned upstream source."""
    linebreak = b"\r\n" if b"\r\n" in data else b"\n"
    first = b"rom_volume1[i] = rom_buff + (0x8000 * (0x100 + i));"
    second_bad = b"\t\trom_volume1[i] = rom_buff + (0x8000 * (0x200 + i));"
    second_good = b"\t\trom_volume2[i] = rom_buff + (0x8000 * (0x200 + i));"
    bad = first + linebreak + second_bad
    good = first + linebreak + second_good
    if data.count(bad) != 1:
        raise ValueError(
            "Expected NC1020 ROM volume pointer typo was not found exactly once; "
            "review upstream before building."
        )
    return data.replace(bad, good, 1)


# The pinned NC1020 core overwrites flash/state files directly. An interrupted
# write (battery loss, process termination, filesystem error) can destroy an
# existing save. Replace the two writers with temp-write-and-rename versions.
# These local modifications deliberately fail closed if upstream changes.
ATOMIC_SAVE_HELPER = r"""
static bool SaveBufferAtomically(const string& filename, const void* content, size_t length) {
    const string staged = filename + ".new";
    FILE* file = fopen(staged.c_str(), "wb");
    if (file == NULL) return false;
    bool ok = fwrite(content, 1, length, file) == length;
    if (ok) ok = fflush(file) == 0;
    // On legacy Kindle FSP/FUSE, fsync may not be implemented; tolerate only
    // the explicit unsupported errors, not actual I/O errors.
    if (ok && fsync(fileno(file)) != 0 &&
        errno != EINVAL && errno != ENOSYS && errno != EOPNOTSUPP) ok = false;
    if (fclose(file) != 0) ok = false;
    if (ok && rename(staged.c_str(), filename.c_str()) == 0) return true;
    remove(staged.c_str());
    return false;
}
"""

SAFE_SAVE_NOR = r"""void SaveNor(){
    uint8_t* temp_buff = (uint8_t*)malloc(NOR_SIZE);
    if (temp_buff == NULL) {
        fprintf(stderr, "NC1020: out of memory when saving NOR flash\n");
        return;
    }
    ProcessBinary(temp_buff, nor_buff, NOR_SIZE);
    if (!SaveBufferAtomically(nc1020_rom.norFlashPath, temp_buff, NOR_SIZE))
        fprintf(stderr, "NC1020: NOR flash could not be safely saved\n");
    free(temp_buff);
}"""

SAFE_LOAD_STATES = r"""void LoadStates(){
    ResetStates();
    FILE* file = fopen(nc1020_rom.statesPath.c_str(), "rb");
    if (file == NULL) return;
    nc1020_states_t saved{};
    const size_t read_size = fread(&saved, 1, sizeof(saved), file);
    const int extra = fgetc(file);
    fclose(file);
    if (read_size != sizeof(saved) || extra != EOF || saved.version != VERSION) {
        fprintf(stderr, "NC1020: ignoring invalid/truncated saved state\n");
        return;
    }
    nc1020_states = saved;
    SwitchVolume();
}"""

SAFE_SAVE_STATES = r"""void SaveStates(){
    if (!SaveBufferAtomically(nc1020_rom.statesPath, &nc1020_states, sizeof(nc1020_states)))
        fprintf(stderr, "NC1020: state could not be safely saved\n");
}"""


def patch_safe_storage(data: bytes) -> bytes:
    """Replace the pinned core's non-atomic writers and unvalidated state loader."""
    linebreak = b"\r\n" if b"\r\n" in data else b"\n"
    include = b"#include <stdlib.h>"
    if data.count(include) != 1:
        raise ValueError("The expected NC1020 C runtime include was not found")
    data = data.replace(include, include + linebreak + b"#include <unistd.h>" + linebreak
                        + b"#include <errno.h>", 1)
    if data.count(b"void SaveNor(){") != 1:
        raise ValueError("Could not locate the expected NC1020 NOR writer")
    data = data.replace(b"void SaveNor(){", ATOMIC_SAVE_HELPER.encode() + b"\nvoid SaveNor(){", 1)
    patches = (
        (rb"(?ms)^void SaveNor\(\)\{.*?^\}", SAFE_SAVE_NOR, b"fwrite(temp_buff"),
        (rb"(?ms)^void LoadStates\(\)\{.*?^\}", SAFE_LOAD_STATES, b"fread(&nc1020_states"),
        (rb"(?ms)^void SaveStates\(\)\{.*?^\}", SAFE_SAVE_STATES, b"fwrite(&nc1020_states"),
    )
    for regex, replacement, signature in patches:
        matches = list(re.finditer(regex, data))
        if len(matches) != 1 or signature not in matches[0].group():
            raise ValueError(f"Pinned upstream storage function signature changed: {regex!r}")
        data = data[:matches[0].start()] + replacement.encode() + data[matches[0].end():]
    return data



def patch_debug_interface(source: bytes, header: bytes):
    """Expose read-only CPU/mapper/LCD diagnostics for direct-game boot research."""
    linebreak = b"\r\n" if b"\r\n" in source else b"\n"
    header_anchor = b"extern void SaveNC1020();"
    if header.count(header_anchor) != 1:
        raise ValueError("Could not locate the NC1020 public API insertion point")
    declaration = b"""struct DebugSnapshot {
    uint16_t pc;
    uint16_t reset_vector;
    uint8_t a, ps, x, y, sp;
    uint8_t nor_bank, volume_select, memory_select;
    uint8_t io05, io06, io08, io09, io0b;
    uint32_t cycles;
    uint32_t lcd_addr;
    bool slept;
    uint8_t keypad[8];
    uint8_t opcodes[8];
};
extern void GetDebugSnapshot(DebugSnapshot* out);
"""
    header = header.replace(header_anchor, declaration + header_anchor, 1)
    if b"void GetDebugSnapshot(DebugSnapshot* out)" in source:
        raise ValueError("DebugSnapshot implementation already exists")
    implementation = b"""void GetDebugSnapshot(DebugSnapshot* out) {
    if (out == NULL) return;
    out->pc = reg_pc;
    out->reset_vector = PeekW(RESET_VEC);
    out->a = reg_a;
    out->ps = reg_ps;
    out->x = reg_x;
    out->y = reg_y;
    out->sp = reg_sp;
    out->nor_bank = ram_io[0x00];
    out->volume_select = ram_io[0x0D];
    out->memory_select = ram_io[0x0A];
    out->io05 = ram_io[0x05];
    out->io06 = ram_io[0x06];
    out->io08 = ram_io[0x08];
    out->io09 = ram_io[0x09];
    out->io0b = ram_io[0x0B];
    out->cycles = cycles;
    out->lcd_addr = lcd_addr;
    out->slept = slept;
    memcpy(out->keypad, keypad_matrix, sizeof(out->keypad));
    for (uint32_t i = 0; i < sizeof(out->opcodes); ++i)
        out->opcodes[i] = Peek((uint16_t)(reg_pc + i));
}
"""
    final_brace = source.rfind(b"}")
    if final_brace < 0 or source[final_brace + 1:].strip():
        raise ValueError("NC1020 source does not end with the expected namespace brace")
    source = source[:final_brace] + implementation + linebreak + source[final_brace:]
    return source, header


def main() -> None:
    out = ROOT / "upstream"
    out.mkdir(parents=True, exist_ok=True)
    for name, expected_blob_sha in FILES.items():
        req = Request(f"{BASE}/{name}", headers={"User-Agent": "Kindle-Golden-Hero-build"})
        with urlopen(req, timeout=30) as response:
            data = response.read()
        git_blob_sha = sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if git_blob_sha != expected_blob_sha:
            raise SystemExit(f"Integrity check failed for {name}: {git_blob_sha}")
        if name == "nc1020.cpp":
            try:
                data = patch_safe_storage(patch_nc1020_source(data))
            except ValueError as exc:
                raise SystemExit(str(exc))
            print("Applied ROM volume pointer and safe-state-save fixes.")
        path = out / name
        path.write_bytes(data)
        print(f"Verified upstream blob {name}: {git_blob_sha}")
    cpp_path, header_path = out / "nc1020.cpp", out / "nc1020.h"
    try:
        patched_cpp, patched_header = patch_debug_interface(
            cpp_path.read_bytes(), header_path.read_bytes())
    except ValueError as exc:
        raise SystemExit(str(exc))
    cpp_path.write_bytes(patched_cpp)
    header_path.write_bytes(patched_header)
    print("Added read-only WQX CPU/mapper/LCD debug snapshot API.")
    print("Fetched emulator source only. No ROM or firmware image was requested.")


if __name__ == "__main__":
    main()
