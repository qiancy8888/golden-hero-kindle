#include "demo.h"
#include <cstdlib>

bool DemoGame::directional(int key){
 return key==0x1A||key==0x1B||key==0x3F||key==0x1F;
}
bool DemoGame::blocked(int tx,int ty){
 if(tx<1||tx>16||ty<0||ty>5)return true;
 if(tx>=3&&tx<=6&&ty<=1)return true; // house
 if(tx==11&&ty==3)return true;       // NPC
 if((tx==2&&ty==4)||(tx==14&&ty==4)||(tx==13&&ty==2))return true; // trees
 return false;
}
bool DemoGame::action(int key){
 if(key==0x13 || key==0x0E){ // MENU or GAME
  menu=!menu;talking=false;return true;
 }
 if(key==0x3B){ // BACK
  if(menu||talking){menu=false;talking=false;return true;}
  return false;
 }
 if(key==0x1D){ // OK
  if(menu){menu=false;return true;}
  talking=(std::abs(x-11)+std::abs(y-3)<=1);
  return true;
 }
 if(!directional(key))return false;
 if(menu||talking){menu=false;talking=false;return true;}
 int nx=x,ny=y;
 if(key==0x1A)--ny;
 if(key==0x1B)++ny;
 if(key==0x3F)--nx;
 if(key==0x1F)++nx;
 if(blocked(nx,ny))return false;
 x=nx;y=ny;++steps;return true;
}
bool DemoGame::down(int key,uint64_t now_ms){
 held_key=directional(key)?key:-1;
 next_repeat_ms=now_ms+400;
 return action(key);
}
void DemoGame::up(int key){
 if(held_key==key)held_key=-1;
}
bool DemoGame::tick(uint64_t now_ms){
 if(held_key<0||now_ms<next_repeat_ms)return false;
 next_repeat_ms=now_ms+280;
 return action(held_key);
}
