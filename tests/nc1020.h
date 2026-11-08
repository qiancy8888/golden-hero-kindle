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
void SaveNC1020();
}
