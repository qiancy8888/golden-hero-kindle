#!/usr/bin/env python3
"""Inspect scene indexes in a privately supplied NC1020 Golden Hero Flash image."""
import argparse
from collections import Counter
import json
from pathlib import Path

BANK_SIZE = 0x8000
GOLDEN_BANK = 0x0C
BANK_BASE = 0x4000
REGION_TABLE = 0x5182
REGION_COUNT = 5
EXPECTED_TABLES = (0x5622, 0x564C, 0x565C, 0x56BE, 0x56D0)


def load_bank(path):
    raw = path.read_bytes()
    if len(raw) != 32 * BANK_SIZE:
        raise ValueError("Expected a 1 MiB NC1020 NOR Flash image")
    bank = raw[GOLDEN_BANK * BANK_SIZE : (GOLDEN_BANK + 1) * BANK_SIZE]
    return bank[0x4000:] + bank[:0x4000]


def read_names(bank, address, delimiter, count):
    names = []
    pos = address - BANK_BASE
    for _ in range(count):
        end = bank.find(delimiter, pos, min(len(bank), pos + 64))
        if end < 0:
            raise ValueError(f"Unterminated name at {pos + BANK_BASE:#06x}")
        names.append(bank[pos:end].decode("gbk"))
        pos = end + 1
    return names


def read_atlas(bank, locations):
    def word(address):
        offset = address - BANK_BASE
        return int.from_bytes(bank[offset : offset + 2], "little")

    tables = tuple(word(REGION_TABLE + i * 2) for i in range(REGION_COUNT))
    if tables != EXPECTED_TABLES:
        raise ValueError("This Flash image has a different scene table layout")
    data_start = word(tables[0])
    scenes = []
    for region, (start, end) in enumerate(zip(tables, tables[1:] + (data_start,)), 1):
        if not BANK_BASE <= start < end < BANK_BASE + len(bank) or (end - start) % 2:
            raise ValueError(f"Invalid pointer table for region {region}")
        for slot, address in enumerate(range(start, end, 2)):
            offset = word(address) - BANK_BASE
            if not data_start - BANK_BASE <= offset < len(bank):
                raise ValueError(f"Invalid scene pointer for {region}:{slot}")
            objects = []
            for pos in range(offset, min(len(bank), offset + 256), 3):
                if bank[pos] == 0xff:
                    break
                if pos + 3 > len(bank):
                    raise ValueError(f"Truncated object record for {region}:{slot}")
                gap, ident, kind = bank[pos : pos + 3]
                if kind > 0x0e:
                    raise ValueError(f"Unexpected object kind for {region}:{slot}")
                obj = {"gap": gap, "id": ident, "kind": kind}
                if kind in (2, 3, 4, 5):
                    obj["destination"] = [kind, ident]
                elif kind in (0x0a, 0x0b):
                    obj["destination"] = [1, ident]
                elif kind == 9 and ident < len(locations):
                    obj["sign"] = locations[ident]
                objects.append(obj)
            else:
                raise ValueError(f"Unterminated object list for {region}:{slot}")
            scenes.append({"region": region, "slot": slot, "address": offset + BANK_BASE, "objects": objects})
    return scenes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("flash", type=Path)
    parser.add_argument("--private-output", type=Path, help="Write parsed object records locally as JSON")
    args = parser.parse_args()
    try:
        bank = load_bank(args.flash)
        locations = read_names(bank, 0x6391, b"\0", 20)
        skills = read_names(bank, 0x8E86, b"\xff", 54)
        items = read_names(bank, 0x9981, b"\xff", 92)
        scenes = read_atlas(bank, locations)
        if args.private_output:
            if args.private_output.resolve() == args.flash.resolve():
                raise ValueError("The analysis output cannot overwrite the Flash image")
            records = {"scenes": scenes, "locations": locations, "skills": skills, "items": items}
            args.private_output.write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n",
                                           encoding="utf-8")
    except (OSError, ValueError) as error:
        parser.exit(2, f"{error}\n")
    counts = Counter(scene["region"] for scene in scenes if scene["objects"])
    print(f"{len(scenes)} scene slots, {sum(counts.values())} non-empty object lists")
    print("Non-empty by region:", ", ".join(f"{region}={count}" for region, count in counts.items()))
    print(f"{len(skills)} skill names, {sum(bool(name) for name in items[1:])} item names")


if __name__ == "__main__":
    main()
