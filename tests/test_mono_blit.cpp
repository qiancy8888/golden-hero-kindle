#include "fb.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main(){
 uint8_t memory[10*8];
 std::memset(memory,0x7f,sizeof(memory));
 FbScreen fb;
 fb.data=memory;fb.w=4;fb.h=3;fb.depth=8;fb.stride=10;
 fb.var.xoffset=1;fb.var.yoffset=1;fb.var.grayscale=1;
 const uint8_t source[1]={0xA0}; // four source pixels: black, white, black, white
 fb.blit_mono_1bpp(0,0,4,1,source,1,0,255);
 const uint8_t* row=memory+1*10+1;
 assert(row[0]==0&&row[1]==255&&row[2]==0&&row[3]==255);
 assert(memory[0]==0x7f&&memory[9]==0x7f);

 std::memset(memory,0x7f,sizeof(memory));
 fb.w=4;fb.h=2;fb.var.xoffset=1;fb.var.yoffset=1;
 fb.blit_mono_1bpp(0,0,2,1,source,2,0,255);
 row=memory+1*10+1;
 assert(row[0]==0&&row[1]==0&&row[2]==255&&row[3]==255);
 const uint8_t* row2=memory+2*10+1;
 assert(std::memcmp(row,row2,4)==0);

 fb.var.grayscale=2;
 std::memset(memory,0x7f,sizeof(memory));
 fb.blit_mono_1bpp(0,0,2,1,source,1,0,255);
 row=memory+1*10+1;
 assert(row[0]==255&&row[1]==0);
 // Fractional nearest-neighbour scaling: 4x1 source 1010 -> 6x2 output.
 std::memset(memory,0x7f,sizeof(memory));
 fb.w=6;fb.h=2;fb.stride=10;fb.var.xoffset=1;fb.var.yoffset=1;fb.var.grayscale=1;
 fb.blit_mono_1bpp_scaled(0,0,4,1,source,6,2,0,255);
 row=memory+1*10+1;
 const uint8_t expected[6]={0,0,255,0,0,255};
 assert(std::memcmp(row,expected,6)==0);
 row2=memory+2*10+1;
 assert(std::memcmp(row,row2,6)==0);

 std::puts("Monochrome LCD blit tests passed: integer/fractional scaling, bit order, offsets and grayscale inversion.");
 return 0;
}
