#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scripts.fetch_upstream import patch_debug_interface

header = b"#pragma once\nnamespace wqx {\nextern void SaveNC1020();\n}\n"
source = b"""namespace wqx {
static uint16_t reg_pc, reg_x, reg_y, reg_sp;
static uint8_t reg_a, reg_ps, ram_io[64], keypad_matrix[8];
static uint32_t cycles, lcd_addr;
static bool slept;
static uint16_t PeekW(uint16_t);
static uint8_t Peek(uint16_t);
static const uint16_t RESET_VEC = 0xFFFC;
}
"""
patched_source, patched_header = patch_debug_interface(source, header)
assert b"struct DebugSnapshot" in patched_header
assert b"GetDebugSnapshot(DebugSnapshot* out)" in patched_header
assert b"out->pc = reg_pc;" in patched_source
assert b"out->reset_vector = PeekW(RESET_VEC);" in patched_source
assert b"out->opcodes[i] = Peek((uint16_t)(reg_pc + i));" in patched_source
assert patched_source.endswith(b"}\n")
print("NC1020 read-only debug API patch tests passed.")
