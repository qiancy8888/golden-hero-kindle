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
void draw_button(FbScreen& fb,const Button& b,bool active,bool refresh=true){
 fb.rect(b.x+2,b.y+2,b.w-4,b.h-4,active?205:255);
 fb.border(b.x,b.y,b.w,b.h,0,3);
 int n=(int)std::strlen(b.label);
 int f=std::max(1,std::min(3,b.w/(n*6+4)));
 label(fb,b.x+(b.w-n*6*f)/2,b.y+(b.h-5*f)/2,b.label,f);
 if(refresh)fb.refresh(b.x,b.y,b.w,b.h,false);
}
int hit_button(int x,int y){
 for(size_t i=0;i<buttons.size();++i){
  const Button& b=buttons[i];
  if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return (int)i;
 }
 return -1;
}
void ui(FbScreen& fb){
 fb.rect(0,0,fb.w,fb.h,255);
 label(fb,std::max(2,(fb.w-11*18)/2),30,"GOLDEN HERO",3);
 fb.border(gx-5,gy-5,160*scale+10,80*scale+10,0,3);
 for(const auto& b:buttons)draw_button(fb,b,false,false);
 fb.refresh(0,0,fb.w,fb.h,true);
}
void lcd_draw(FbScreen& fb,const uint8_t* frame){
 fb.blit_mono_1bpp(gx,gy,160,80,frame,scale,0,255);
 fb.refresh(gx,gy,160*scale,80*scale,false);
}
void lcd_set_pixel(uint8_t* frame,int x,int y){
 if(x<0||x>=160||y<0||y>=80)return;
 const int bit=y*160+x;
 frame[bit>>3]|=(uint8_t)(1u<<(7-(bit&7)));
}
void lcd_draw_text(uint8_t* frame,int x,int y,const char* text){
 for(;*text;++text,x+=6){
  const char* bits=glyph(*text);
  for(int row=0;row<5;++row)for(int col=0;col<5;++col)
   if(bits[row*5+col]=='1')lcd_set_pixel(frame,x+col,y+row);
 }
}
void make_demo_lcd(uint8_t* frame){
 std::memset(frame,0,1600);
 for(int x=2;x<158;++x){lcd_set_pixel(frame,x,2);lcd_set_pixel(frame,x,77);}
 for(int y=2;y<78;++y){lcd_set_pixel(frame,2,y);lcd_set_pixel(frame,157,y);}
 for(int x=10;x<150;++x)lcd_set_pixel(frame,x,25);
 lcd_draw_text(frame,47,12,"GOLDEN HERO");
 lcd_draw_text(frame,41,34,"TOUCH UI TEST");
 lcd_draw_text(frame,50,55,"UP DOWN OK");
}
int run_touch_test(bool show_lcd){
 std::string error;
 FbScreen fb;
 if(!fb.open_fb(error)){std::fprintf(stderr,"Framebuffer: %s\n",error.c_str());return 10;}
 TouchInput touch;
 if(!touch.open_touch(error)){std::fprintf(stderr,"Touchscreen: %s\n",error.c_str());fb.close_fb();return 11;}

 // Preserve the screen that was visible before Awesome/Pillow were paused.
 // Resuming the window manager alone does not guarantee a Kindle UI repaint.
 const size_t region=(size_t)fb.stride*(size_t)fb.h;
 unsigned char* visible=fb.data+(size_t)fb.stride*(size_t)fb.var.yoffset;
 const std::vector<unsigned char> backup(visible,visible+region);
 std::fprintf(stderr,"Original framebuffer snapshot captured (%zu bytes).\n",region);

 touch.trace_events=true;
 layout(fb.w,fb.h);
 ui(fb);
 if(show_lcd){
  uint8_t demo[1600]={};
  make_demo_lcd(demo);
  lcd_draw(fb,demo);
  std::fprintf(stderr,"ROM-free LCD layout preview: 160x80 source rendered at %dx scale.\n",scale);
 }else{
  label(fb,gx+(160*scale-10*18)/2,gy+28*scale,"TOUCH TEST",3);
  label(fb,gx+(160*scale-12*12)/2,gy+52*scale,"TAP OR HOLD",2);
  fb.refresh(gx,gy,160*scale,80*scale,false);
 }
 std::fprintf(stderr,"Controls test ready: tap or hold any virtual button; EXIT ends on release; timeout=45s.\n");

 interrupted=0;
 std::signal(SIGINT,on_signal);
 std::signal(SIGTERM,on_signal);
 const uint64_t started=clock_ms();
 int active_button=-1;
 bool exit_armed=false;
 bool exit_requested=false;
 while(!exit_requested&&!interrupted&&clock_ms()-started<45000){
  TouchPoint point;
  if(touch.poll_point(fb.w,fb.h,point)){
   const int hit=point.valid?hit_button(point.x,point.y):-1;
   if(point.pressed){
    active_button=hit;
    exit_armed=false;
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     draw_button(fb,b,true);
     std::fprintf(stderr,"CONTROL DOWN label=%s key=%d x=%d y=%d\n",b.label,b.key,point.x,point.y);
     if(b.key==-1)exit_armed=true;
    }else std::fprintf(stderr,"CONTROL DOWN outside x=%d y=%d\n",point.x,point.y);
   }else if(point.active&&point.moved&&hit!=active_button){
    if(active_button>=0)draw_button(fb,buttons[(size_t)active_button],false);
    active_button=hit;
    exit_armed=false;
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     draw_button(fb,b,true);
     std::fprintf(stderr,"CONTROL DRAG label=%s key=%d x=%d y=%d\n",b.label,b.key,point.x,point.y);
     if(b.key==-1)exit_armed=true;
    }else std::fprintf(stderr,"CONTROL DRAG outside x=%d y=%d\n",point.x,point.y);
   }
   if(point.released){
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     std::fprintf(stderr,"CONTROL UP label=%s key=%d x=%d y=%d\n",b.label,b.key,point.x,point.y);
     if(b.key==-1&&exit_armed)exit_requested=true;
     draw_button(fb,b,false);
    }else std::fprintf(stderr,"CONTROL UP outside x=%d y=%d\n",point.x,point.y);
    active_button=-1;
    exit_armed=false;
   }
  }
  usleep(15000);
 }
 if(active_button>=0)draw_button(fb,buttons[(size_t)active_button],false);
 const uint64_t elapsed=clock_ms()-started;
 const char* reason=exit_requested?"EXIT button":interrupted?"signal":"45-second timeout";
 std::fprintf(stderr,"Controls test finished: reason=%s elapsed_ms=%llu events=%lu taps=%lu\n",
  reason,(unsigned long long)elapsed,touch.event_count,touch.tap_count);
 touch.close_touch();

 // Restore framebuffer pixels and force an EPDC refresh before returning to KUAL.
 std::memcpy(visible,backup.data(),region);
 const bool restored=fb.refresh(0,0,fb.w,fb.h,true);
 std::fprintf(stderr,"Previous framebuffer restore refresh: %s\n",restored?"OK":"FAILED");
 fb.close_fb();
 return restored?0:12;
}
int run_controls_test(){return run_touch_test(false);}
int run_ui_test(){return run_touch_test(true);}

void help(){
 std::puts("Golden Hero for KPW1 [DEVELOPMENT BUILD]\n"
           "Usage: golden_hero [--display-test] [--controls-test] [--ui-test] [--check-files] [--touch-swap-xy] [--touch-flip-x] [--touch-flip-y]\n"
           "ROM files: roms/obj_lu.bin and roms/nc1020.fls\n"
           "Needs /dev/fb0 and an accessible /dev/input/eventN touchscreen.\n"
           "Do NOT run on a PC except --check-files or --help.");
}
}

int run_display_test();

int main(int argc,char** argv){
 bool check=false,display_test=false,controls_test=false,ui_test=false,swap=false,flipx=false,flipy=false;
 for(int i=1;i<argc;++i){
  if(!strcmp(argv[i],"--help")){help();return 0;}
  if(!strcmp(argv[i],"--check-files"))check=true;
  else if(!strcmp(argv[i],"--display-test"))display_test=true;
  else if(!strcmp(argv[i],"--controls-test"))controls_test=true;
  else if(!strcmp(argv[i],"--ui-test"))ui_test=true;
  else if(!strcmp(argv[i],"--touch-swap-xy"))swap=true;
  else if(!strcmp(argv[i],"--touch-flip-x"))flipx=true;
  else if(!strcmp(argv[i],"--touch-flip-y"))flipy=true;
  else{std::fprintf(stderr,"Unknown argument: %s\n",argv[i]);return 2;}
 }
 if(display_test) return run_display_test();
 if(controls_test) return run_controls_test();
 if(ui_test) return run_ui_test();
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
 int active_button=-1,held_key=-1;
 while(!interrupted){
  const uint32_t slice=25;
  wqx::RunTimeSlice(slice,false);
  TouchPoint point;
  if(touch.poll_point(fb.w,fb.h,point)){
   const int hit=point.valid?hit_button(point.x,point.y):-1;
   if(point.pressed){
    active_button=hit;
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     draw_button(fb,b,true);
     if(b.key==-1)interrupted=1;
     else if(b.key==-2){
      wqx::SaveNC1020();
      std::fprintf(stderr,"Save requested (NOR + state); check logs for any storage errors.\n");
     }else{
      held_key=b.key;
      wqx::SetKey((uint8_t)held_key,true);
      std::fprintf(stderr,"Key down: %s (0x%02X)\n",b.label,(unsigned)held_key);
     }
    }
   }else if(point.active&&point.moved&&hit!=active_button){
    if(held_key>=0){wqx::SetKey((uint8_t)held_key,false);held_key=-1;}
    if(active_button>=0)draw_button(fb,buttons[(size_t)active_button],false);
    active_button=hit;
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     draw_button(fb,b,true);
     if(b.key>=0){
      held_key=b.key;
      wqx::SetKey((uint8_t)held_key,true);
      std::fprintf(stderr,"Key down (drag): %s (0x%02X)\n",b.label,(unsigned)held_key);
     }
    }
   }
   if(point.released){
    if(held_key>=0){
     wqx::SetKey((uint8_t)held_key,false);
     std::fprintf(stderr,"Key up (0x%02X)\n",(unsigned)held_key);
     held_key=-1;
    }
    if(active_button>=0)draw_button(fb,buttons[(size_t)active_button],false);
    active_button=-1;
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
 if(held_key>=0)wqx::SetKey((uint8_t)held_key,false);
 wqx::SaveNC1020();
 touch.close_touch();fb.close_fb();return 0;
}
