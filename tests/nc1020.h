#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>
namespace wqx {
struct WqxRom {
 std::string romPath;
 std::string norFlashPath;
 std::string statesPath;
};
void Initialize(WqxRom);
void Reset();
void SetKey(uint8_t,bool);
void RunTimeSlice(uint32_t,bool);
bool CopyLcdBuffer(uint8_t*);
void LoadNC1020();
struct DebugSnapshot {
 uint16_t pc, reset_vector;
 uint8_t a, ps, x, y, sp;
 uint8_t nor_bank, volume_select, memory_select;
 uint8_t io05, io06, io08, io09, io0b;
 uint32_t cycles, lcd_addr;
 bool slept;
 uint8_t keypad[8], opcodes[8];
};
void GetDebugSnapshot(DebugSnapshot*);
void SaveNC1020();
}
