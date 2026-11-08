#pragma once
#include <cstdint>

// ROM-free input and e-ink refresh test, NOT the Golden Hero game logic.
// Keeping this tiny test model independent makes movement reproducible on a PC.
struct DemoGame {
 int x=10, y=3, steps=0;
 bool menu=false, talking=false;
 int held_key=-1;
 uint64_t next_repeat_ms=0;
 static bool directional(int key);
 static bool blocked(int x,int y);
 bool action(int key);
 bool down(int key,uint64_t now_ms);
 void up(int key);
 bool tick(uint64_t now_ms);
};
