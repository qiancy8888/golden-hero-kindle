#include "nc1020.h"
#include <cstring>
namespace wqx {
static unsigned ticks=0;
void Initialize(WqxRom) {}
void Reset() {ticks=0;}
void SetKey(uint8_t,bool) {}
void RunTimeSlice(uint32_t ms, bool) {ticks+=ms;}
bool CopyLcdBuffer(uint8_t* out) {
 std::memset(out, 0, 1600);
 for(int y=10;y<70;++y)for(int x=10;x<150;++x)
  if((x+y+ticks/100)%8==0)out[(y*160+x)/8]|=1u<<(7-(x%8));
 return true;
}
void LoadNC1020() {}
void SaveNC1020() {}
}
