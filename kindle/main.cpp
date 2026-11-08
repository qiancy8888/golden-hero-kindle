#include "nc1020.h"
#include "fb.h"
#include "touch.h"
#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace {
volatile sig_atomic_t interrupted=0;
void on_signal(int) {interrupted=1;}
uint64_t clock_ms() {struct timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);return uint64_t(ts.tv_sec)*1000+ts.tv_nsec/1000000;}
void nap_ms(unsigned ms) {usleep(ms*1000);}

const char* filename_rom="roms/obj_lu.bin";
const char* filename_initial_flash="roms/nc1020.fls";
const char* filename_flash="data/nc1020.fls";
const char* filename_state="data/nc1020.sts";

bool check_size(const char* name, off_t expected) {
  struct stat st{};
  if(stat(name,&st)!=0){std::fprintf(stderr,"Missing %s\n",name);return false;}
  if(st.st_size!=expected){std::fprintf(stderr,"Unexpected size %s: got %lld, expected %lld\n",
      name,(long long)st.st_size,(long long)expected);return false;}
  return true;
}
bool copy_file(const char* src,const char* dst) {
 FILE* in=fopen(src,"rb"); if(!in)return false;
 FILE* out=fopen(dst,"wb");if(!out){fclose(in);return false;}
 char buf[8192]; size_t n; bool good=true;
 while((n=fread(buf,1,sizeof(buf),in))>0) if(fwrite(buf,1,n,out)!=n){good=false;break;}
 if(ferror(in))good=false;
 if(fclose(out)!=0)good=false;
 fclose(in);return good;
}
struct Button {int x,y,w,h,key;const char* label;};
std::vector<Button> buttons;
int gx=0,gy=0,scale=1;
void layout(int w,int h){
  // KPW1 is 758x1024. Keep the 160x80 LCD at 4x integer scale.
  scale=std::max(1,std::min((w-22)/160,(h/2-10)/80));
  gx=(w-160*scale)/2;
  gy=std::max(44,h/13);

  buttons.clear();
  const int u=std::max(76,std::min(96,w/8));
  const int dpad_cx=w*27/100;
  const int dpad_cy=h*68/100;
  buttons.push_back({dpad_cx-u/2,dpad_cy-3*u/2,u,u,0x1A,"UP"});
  buttons.push_back({dpad_cx-u/2,dpad_cy+u/2,u,u,0x1B,"DOWN"});
  buttons.push_back({dpad_cx-3*u/2,dpad_cy-u/2,u,u,0x3F,"LEFT"});
  buttons.push_back({dpad_cx+u/2,dpad_cy-u/2,u,u,0x1F,"RIGHT"});

  // Full touch keypad: all keys exposed by the upstream GameShell mapping.
  const int grid_x=w*50/100;
  const int gap=12;
  const int bw=(w-grid_x-18-gap)/2;
  const int bh=62;
  const int top=h*43/100;
  const int step=78;
  const struct {int key;const char* label;} actions[]={
    {0x10,"FLY"},{0x13,"MENU"},
    {0x1D,"OK"},{0x3B,"BACK"},
    {0x08,"MAIN"},{0x0E,"GAME"},
    {0x31,"X"},{0x25,"Y"},
    {0x37,"PGUP"},{0x1E,"PGDN"},
    {-2,"SAVE"},{-1,"EXIT"}
  };
  for(int i=0;i<12;i++){
    const int col=i%2,row=i/2;
    buttons.push_back({grid_x+col*(bw+gap),top+row*step,bw,bh,
                       actions[i].key,actions[i].label});
  }
}
const char* glyph(char c){
 switch(c){
 case 'C':return "0111110000100001000001111";
 case 'H':return "1000110001111111000110001";
 case 'A':return "0111010001111111000110001";
 case 'B':return "1111010001111101000111110";
 case 'D':return "1111010001100011000111110";
 case 'E':return "1111110000111101000011111";
 case 'F':return "1111110000111101000010000";
 case 'G':return "0111110000101111000101111";
 case 'I':return "1111100100001000010011111";
 case 'K':return "1000110010111001001010001";
 case 'L':return "1000010000100001000011111";
 case 'M':return "1000111011101011000110001";
 case 'N':return "1000111001101011001110001";
 case 'O':return "0111010001100011000101110";
 case 'P':return "1111010001111101000010000";
 case 'R':return "1111010001111101010010001";
 case 'S':return "0111110000011100000111110";
 case 'T':return "1111100100001000010010010";
 case 'U':return "1000110001100011000101110";
 case 'V':return "1000110001100010101000100";
 case 'W':return "1000110001101011101110001";
 case 'X':return "1000101010001000101010001";
 case 'Y':return "1000101010001000010000100";
 default:return "0000000000000000000000000";
 }
}
void label(FbScreen& fb,int x,int y,const char* s,int s2=2) {
 for(;*s;s++,x+=6*s2){const char* bits=glyph(*s);
  for(int row=0;row<5;row++)for(int col=0;col<5;col++)
   if(bits[row*5+col]=='1')fb.rect(x+col*s2,y+row*s2,s2,s2,0);
 }
}
void ui(FbScreen& fb){
 fb.rect(0,0,fb.w,fb.h,255);
 label(fb,std::max(2,(fb.w-11*18)/2),30,"GOLDEN HERO",3);
 fb.border(gx-5,gy-5,160*scale+10,80*scale+10,0,3);
 for(const auto& b:buttons){
  fb.border(b.x,b.y,b.w,b.h,0,3);
  int n=strlen(b.label);int f=std::max(1,std::min(3,b.w/(n*6+4)));
  label(fb,b.x+(b.w-n*6*f)/2,b.y+(b.h-5*f)/2,b.label,f);
 }
 fb.refresh(0,0,fb.w,fb.h,true);
}
void lcd_draw(FbScreen& fb,const uint8_t* frame){
 for(int y=0;y<80;++y){
  for(int x=0;x<160;++x){
   int pos=y*160+x;
   const bool black=(frame[pos>>3] & (1<<(7-(pos&7))))!=0;
   fb.rect(gx+x*scale,gy+y*scale,scale,scale,black?0:255);
  }
 }
 fb.refresh(gx,gy,160*scale,80*scale,false);
}
void help(){
 std::puts("Golden Hero for KPW1 [DEVELOPMENT BUILD]\n"
           "Usage: golden_hero [--display-test] [--check-files] [--touch-swap-xy] [--touch-flip-x] [--touch-flip-y]\n"
           "ROM files: roms/obj_lu.bin and roms/nc1020.fls\n"
           "Needs /dev/fb0 and an accessible /dev/input/eventN touchscreen.\n"
           "Do NOT run on a PC except --check-files or --help.");
}
}

int run_display_test();

int main(int argc,char** argv){
 bool check=false,display_test=false,swap=false,flipx=false,flipy=false;
 for(int i=1;i<argc;++i){
  if(!strcmp(argv[i],"--help")){help();return 0;}
  if(!strcmp(argv[i],"--check-files"))check=true;
  else if(!strcmp(argv[i],"--display-test"))display_test=true;
  else if(!strcmp(argv[i],"--touch-swap-xy"))swap=true;
  else if(!strcmp(argv[i],"--touch-flip-x"))flipx=true;
  else if(!strcmp(argv[i],"--touch-flip-y"))flipy=true;
  else{std::fprintf(stderr,"Unknown argument: %s\n",argv[i]);return 2;}
 }
 if(display_test) return run_display_test();
 bool ok=check_size(filename_rom,0x8000*0x300) && check_size(filename_initial_flash,0x8000*0x20);
 if(check)return ok?0:3;
 if(!ok){std::fprintf(stderr,"Required NC1020 image(s) not found or wrong size; aborting.\n");return 3;}
 // Preserve supplied initial flash. All game changes go to data/nc1020.fls.
 mkdir("data",0755);
 struct stat flash_info{};
 if(stat(filename_flash,&flash_info)!=0){
  if(!copy_file(filename_initial_flash,filename_flash)){
   std::fprintf(stderr,"Cannot initialize data/nc1020.fls\n");return 4;
  }
 }
 if(!check_size(filename_flash,0x8000*0x20))return 4;
 std::signal(SIGINT,on_signal); std::signal(SIGTERM,on_signal);
 wqx::WqxRom rom{filename_rom,filename_flash,filename_state};
 wqx::Initialize(rom);
 wqx::LoadNC1020();
 FbScreen fb;std::string error;
 if(!fb.open_fb(error)) {std::fprintf(stderr,"Framebuffer: %s\n",error.c_str());return 5;}
 TouchInput touch;
 if(!touch.open_touch(error)){
  std::fprintf(stderr,"Touchscreen: %s\n",error.c_str());fb.close_fb();return 6;
 }
 touch.swap_xy=swap;touch.flip_x=flipx;touch.flip_y=flipy;
 layout(fb.w,fb.h); ui(fb);
 uint8_t lcd[1600]={};uint8_t previous[1600]={};bool was_drawn=false;
 uint64_t last_frame=0,last_full=clock_ms(),next=clock_ms();
 while(!interrupted){
  const uint32_t slice=25;
  wqx::RunTimeSlice(slice,false);
  int x,y;
  if(touch.poll_tap(fb.w,fb.h,x,y)){
   for(const auto& b:buttons){
    if(x>=b.x && x<b.x+b.w && y>=b.y && y<b.y+b.h){
     if(b.key==-1)interrupted=1;
     else if(b.key==-2){wqx::SaveNC1020();std::fprintf(stderr,"Save requested (NOR + state); check logs for any storage errors.\n");}
     else {wqx::SetKey((uint8_t)b.key,true);wqx::RunTimeSlice(85,false);
           wqx::SetKey((uint8_t)b.key,false);}
     break;
    }
   }
  }
  uint64_t now=clock_ms();
  if(now-last_frame>=180){
   if(wqx::CopyLcdBuffer(lcd) && (!was_drawn||memcmp(lcd,previous,1600)!=0)){
    lcd_draw(fb,lcd);memcpy(previous,lcd,1600);was_drawn=true;
   }
   last_frame=now;
  }
  // Full periodic flash to clear ghosting without flashing on each game step.
  if(now-last_full>=180000){fb.refresh(0,0,fb.w,fb.h,true);last_full=now;}
  next+=slice;now=clock_ms();
  if(next>now)nap_ms((unsigned)std::min<uint64_t>(next-now,50));
  else if(now-next>250)next=now;
 }
 wqx::SaveNC1020();
 touch.close_touch();fb.close_fb();return 0;
}
