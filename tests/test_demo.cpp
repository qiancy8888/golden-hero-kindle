#include "demo.h"
#include <cassert>
#include <cstdio>
int main(){
 DemoGame d;
 assert(d.x==10 && d.y==3);
 assert(DemoGame::blocked(11,3)); // NPC
 assert(DemoGame::blocked(4,0));  // house
 assert(!DemoGame::blocked(10,4));
 assert(!d.down(0x1F,100)); // wall/NPC to the right
 assert(d.x==10 && d.steps==0);
 d.up(0x1F);
 assert(d.down(0x1D,200)); // OK next to NPC
 assert(d.talking);
 d.up(0x1D);
 assert(d.down(0x3B,300)); // BACK closes dialog
 assert(!d.talking);
 d.up(0x3B);
 assert(d.down(0x1B,400)); // DOWN starts moving
 assert(d.y==4 && d.steps==1);
 assert(!d.tick(700)); // before first repeat
 assert(d.tick(800));  // repeat at 400ms
 assert(d.y==5 && d.steps==2);
 d.up(0x1B);
 assert(!d.tick(2000)); // released, no repeat
 assert(d.down(0x13,2100) && d.menu);
 d.up(0x13);
 assert(d.down(0x3B,2200) && !d.menu);
 std::puts("ROM-free town demo checks passed (movement/collision/hold/release/interaction/menu).");
 return 0;
}
