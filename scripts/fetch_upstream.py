#!/usr/bin/env python3
"""Fetch only the BSD-licensed NC1020 emulator source at a pinned commit.

The upstream repository also contains game/firmware images. This script never
requests or downloads those binary files.
"""
from hashlib import sha1
from pathlib import Path
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
                data = patch_nc1020_source(data)
            except ValueError as exc:
                raise SystemExit(str(exc))
            print("Applied local NC1020 ROM volume 2 pointer fix.")
        path = out / name
        path.write_bytes(data)
        print(f"Verified upstream blob {name}: {git_blob_sha}")
    print("Fetched emulator source only. No ROM or firmware image was requested.")


if __name__ == "__main__":
    main()
