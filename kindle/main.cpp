#include "nc1020.h"
#include "fb.h"
#include "touch.h"
#include "demo.h"
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

// KUAL points this at /mnt/us/documents/GoldenHeroPrivate.
// Desktop/developer runs default to the current working directory.
std::string private_root=".";
std::string rom_dir, data_dir;
std::string filename_rom, filename_initial_flash, filename_flash, filename_state;

void configure_storage_paths() {
  const char* configured=std::getenv("GOLDEN_HERO_PRIVATE_DIR");
  if(configured && *configured) private_root=configured;
  while(private_root.size()>1 && private_root[private_root.size()-1]=='/')
    private_root.erase(private_root.size()-1);
  rom_dir=private_root+"/roms";
  data_dir=private_root+"/data";
  filename_rom=rom_dir+"/obj_lu.bin";
  filename_initial_flash=rom_dir+"/nc1020.fls";
  filename_flash=data_dir+"/nc1020.fls";
  filename_state=data_dir+"/nc1020.sts";
}

bool check_size(const std::string& name, off_t expected) {
  struct stat st{};
  if(stat(name.c_str(),&st)!=0){std::fprintf(stderr,"Missing %s\n",name.c_str());return false;}
  if(st.st_size!=expected){std::fprintf(stderr,"Unexpected size %s: got %lld, expected %lld\n",
      name.c_str(),(long long)st.st_size,(long long)expected);return false;}
  return true;
}
bool copy_file(const std::string& src,const std::string& dst) {
 FILE* in=fopen(src.c_str(),"rb"); if(!in)return false;
 FILE* out=fopen(dst.c_str(),"wb");if(!out){fclose(in);return false;}
 char buf[8192]; size_t n; bool good=true;
 while((n=fread(buf,1,sizeof(buf),in))>0) if(fwrite(buf,1,n,out)!=n){good=false;break;}
 if(ferror(in))good=false;
 if(fclose(out)!=0)good=false;
 fclose(in);return good;
}
bool ensure_directory(const std::string& path) {
 if(mkdir(path.c_str(),0755)==0)return true;
 struct stat st{};
 return stat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
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
// Refresh only the bounding box of changed NC1020 1bpp bytes. The direct
// framebuffer blit still replaces the complete 640x320 viewport in memory.
bool lcd_draw_diff(FbScreen& fb,const uint8_t* previous,const uint8_t* next){
 int xmin=160,ymin=80,xmax=0,ymax=0;
 for(int byte=0;byte<1600;++byte){
  if(previous[byte]==next[byte])continue;
  const int y=byte/20;
  const int x=(byte%20)*8;
  xmin=std::min(xmin,x);xmax=std::max(xmax,x+8);
  ymin=std::min(ymin,y);ymax=std::max(ymax,y+1);
 }
 if(xmax<=xmin||ymax<=ymin)return false;
 fb.blit_mono_1bpp(gx,gy,160,80,next,scale,0,255);
 const bool ok=fb.refresh(gx+xmin*scale,gy+ymin*scale,(xmax-xmin)*scale,(ymax-ymin)*scale,false);
 std::fprintf(stderr,"LCD dirty rect x=%d..%d y=%d..%d refresh=%d\n",xmin,xmax,ymin,ymax,(int)ok);
 return true;
}

void lcd_box(uint8_t* buffer,int x,int y,int w,int h){
 for(int i=0;i<w;++i){lcd_set_pixel(buffer,x+i,y);lcd_set_pixel(buffer,x+i,y+h-1);}
 for(int j=0;j<h;++j){lcd_set_pixel(buffer,x,y+j);lcd_set_pixel(buffer,x+w-1,y+j);}
}
void lcd_fill(uint8_t* buffer,int x,int y,int w,int h,bool black){
 for(int j=0;j<h;++j)for(int i=0;i<w;++i){
  const int tx=x+i,ty=y+j;
  if(tx<0||tx>=160||ty<0||ty>=80)continue;
  const int n=ty*160+tx;
  if(black)buffer[n>>3]|=(uint8_t)(1u<<(7-(n&7)));
  else buffer[n>>3]&=(uint8_t)~(1u<<(7-(n&7)));
 }
}
void lcd_tree(uint8_t* frame,int col,int row){
 const int x=8+col*8,y=18+row*8;
 lcd_fill(frame,x+3,y,2,7,true);
 lcd_fill(frame,x+1,y+2,6,2,true);
}
void render_town_demo(uint8_t* frame,const DemoGame& game){
 // This miniature test scene is original and contains no proprietary game art.
 std::memset(frame,0,1600);
 lcd_draw_text(frame,42,4,"TOWN DEMO");
 for(int x=4;x<156;++x)lcd_set_pixel(frame,x,13);
 lcd_box(frame,4,16,152,56);
 // A house, trees and an NPC form fixed collision landmarks.
 lcd_box(frame,8+3*8,18,32,16);
 lcd_fill(frame,8+4*8,29,6,4,true);
 lcd_tree(frame,2,4);
 lcd_tree(frame,13,2);
 lcd_tree(frame,14,4);
 const int npc_x=8+11*8,npc_y=18+3*8;
 lcd_fill(frame,npc_x+2,npc_y,4,4,true);
 lcd_set_pixel(frame,npc_x+1,npc_y+5);
 lcd_set_pixel(frame,npc_x+6,npc_y+5);
 // 5x7 filled player with a white face dot and legs.
 const int x=8+game.x*8,y=18+game.y*8;
 lcd_fill(frame,x+1,y,6,6,true);
 lcd_fill(frame,x+3,y+2,2,2,false);
 lcd_set_pixel(frame,x+1,y+7);lcd_set_pixel(frame,x+6,y+7);
 lcd_draw_text(frame,6,74,"OK TALK MENU HELP");
 if(game.menu||game.talking){
  lcd_fill(frame,22,22,118,39,false);
  lcd_box(frame,22,22,118,39);
  lcd_draw_text(frame,35,30,game.menu?"DEMO PAUSE":"NPC HELLO");
  lcd_draw_text(frame,32,43,game.menu?"USE DIR KEYS":"WELCOME");
  lcd_draw_text(frame,32,53,"BACK RETURN");
 }
}
int run_touch_test(bool show_lcd,bool playable_demo=false){
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
 DemoGame demo;
 uint8_t prev_lcd[1600]={};
 bool demo_has_frame=false;
 const auto update_demo=[&](){
  uint8_t fresh[1600];
  render_town_demo(fresh,demo);
  if(demo_has_frame)lcd_draw_diff(fb,prev_lcd,fresh);
  else{lcd_draw(fb,fresh);demo_has_frame=true;}
  std::memcpy(prev_lcd,fresh,sizeof(prev_lcd));
  std::fprintf(stderr,"DEMO position=(%d,%d) steps=%d menu=%d talking=%d held=%d\n",
    demo.x,demo.y,demo.steps,(int)demo.menu,(int)demo.talking,demo.held_key);
 };
 if(playable_demo){
  update_demo();
  std::fprintf(stderr,"ROM-free town demo: arrows move, hold repeats, OK interacts, MENU overlays.\n");
 }else if(show_lcd){
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
     if(playable_demo&&b.key>=0&&demo.down(b.key,clock_ms()))update_demo();
     if(b.key==-1)exit_armed=true;
    }else std::fprintf(stderr,"CONTROL DOWN outside x=%d y=%d\n",point.x,point.y);
   }else if(point.active&&point.moved&&hit!=active_button){
    if(playable_demo&&active_button>=0&&buttons[(size_t)active_button].key>=0)
      demo.up(buttons[(size_t)active_button].key);
    if(active_button>=0)draw_button(fb,buttons[(size_t)active_button],false);
    active_button=hit;
    exit_armed=false;
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     draw_button(fb,b,true);
     std::fprintf(stderr,"CONTROL DRAG label=%s key=%d x=%d y=%d\n",b.label,b.key,point.x,point.y);
     if(playable_demo&&b.key>=0&&demo.down(b.key,clock_ms()))update_demo();
     if(b.key==-1)exit_armed=true;
    }else std::fprintf(stderr,"CONTROL DRAG outside x=%d y=%d\n",point.x,point.y);
   }
   if(point.released){
    if(active_button>=0){
     const Button& b=buttons[(size_t)active_button];
     std::fprintf(stderr,"CONTROL UP label=%s key=%d x=%d y=%d\n",b.label,b.key,point.x,point.y);
     if(playable_demo&&b.key>=0)demo.up(b.key);
     if(b.key==-1&&exit_armed)exit_requested=true;
     draw_button(fb,b,false);
    }else std::fprintf(stderr,"CONTROL UP outside x=%d y=%d\n",point.x,point.y);
    active_button=-1;
    exit_armed=false;
   }
  }
  if(playable_demo&&demo.tick(clock_ms()))update_demo();
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
int run_demo_test(){return run_touch_test(true,true);}

void help(){
 std::puts("Golden Hero for KPW1 [DEVELOPMENT BUILD]\n"
           "Usage: golden_hero [--display-test] [--controls-test] [--ui-test] [--demo-test] [--safe-game-test] [--check-files] [--touch-swap-xy] [--touch-flip-x] [--touch-flip-y]\n"
           "ROM files: $GOLDEN_HERO_PRIVATE_DIR/roms/obj_lu.bin and nc1020.fls\n"
           "KUAL sets GOLDEN_HERO_PRIVATE_DIR to /mnt/us/documents/GoldenHeroPrivate.\n"
           "Needs /dev/fb0 and an accessible /dev/input/eventN touchscreen.\n"
           "Do NOT run on a PC except --check-files or --help.");
}
}

int run_display_test();

int main(int argc,char** argv){
 bool check=false,display_test=false,controls_test=false,ui_test=false,demo_test=false,safe_game_test=false,swap=false,flipx=false,flipy=false;
 for(int i=1;i<argc;++i){
  if(!strcmp(argv[i],"--help")){help();return 0;}
  if(!strcmp(argv[i],"--check-files"))check=true;
  else if(!strcmp(argv[i],"--display-test"))display_test=true;
  else if(!strcmp(argv[i],"--controls-test"))controls_test=true;
  else if(!strcmp(argv[i],"--ui-test"))ui_test=true;
  else if(!strcmp(argv[i],"--demo-test"))demo_test=true;
  else if(!strcmp(argv[i],"--safe-game-test"))safe_game_test=true;
  else if(!strcmp(argv[i],"--touch-swap-xy"))swap=true;
  else if(!strcmp(argv[i],"--touch-flip-x"))flipx=true;
  else if(!strcmp(argv[i],"--touch-flip-y"))flipy=true;
  else{std::fprintf(stderr,"Unknown argument: %s\n",argv[i]);return 2;}
 }
 configure_storage_paths();
 if(display_test) return run_display_test();
 if(controls_test) return run_controls_test();
 if(ui_test) return run_ui_test();
 if(demo_test) return run_demo_test();
 std::fprintf(stderr,"Private NC1020 data directory: %s\n",private_root.c_str());
 bool ok=check_size(filename_rom,0x8000*0x300) && check_size(filename_initial_flash,0x8000*0x20);
 if(check)return ok?0:3;
 if(!safe_game_test) {
  std::fprintf(stderr,"ERROR: unprotected persistent game mode is disabled after KPW1 reboot report. Use the guarded KUAL safe-game test.\n");
  return 20;
 }
 if(!ok){std::fprintf(stderr,"Required NC1020 image(s) not found or wrong size; aborting.\n");return 3;}
 // Safe game test: always load pristine user-provided ROM and NOR images.
 // Never read a possibly corrupted previous saved state or write any save.
 filename_flash=filename_initial_flash;
 filename_state="/tmp/golden-hero-no-existing-state-"+std::to_string((long long)getpid())+".sts";
 std::fprintf(stderr,"SAFE GAME TEST: 30s limit, MAIN/SAVE disabled, no persistent reads/writes, NOR and state NOT saved.\n");
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
 // Match the v0.8.1 proven EXIT recovery path in the real emulator too.
 const size_t display_bytes=(size_t)fb.stride*(size_t)fb.h;
 unsigned char* old_visible=fb.data+(size_t)fb.stride*(size_t)fb.var.yoffset;
 const std::vector<unsigned char> old_frame(old_visible,old_visible+display_bytes);
 std::fprintf(stderr,"SAFE GAME TEST original framebuffer snapshot: %zu bytes\n",display_bytes);
 layout(fb.w,fb.h); ui(fb);
 uint8_t lcd[1600]={};uint8_t previous[1600]={};bool was_drawn=false;
 uint64_t last_frame=0,last_full=clock_ms(),next=clock_ms();
 const uint64_t started=clock_ms();
 int active_button=-1,held_key=-1;
 while(!interrupted && clock_ms()-started<30000){
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
     if(b.key==-1){interrupted=1;std::fprintf(stderr,"SAFE GAME TEST: EXIT requested.\n");}
     else if(b.key==-2 || b.key==0x08){
      std::fprintf(stderr,"SAFE GAME TEST: %s is disabled to avoid reset and persistent writes.\n",b.label);
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
     if(b.key>=0 && b.key!=0x08){
      held_key=b.key;
      wqx::SetKey((uint8_t)held_key,true);
      std::fprintf(stderr,"Key down (drag): %s (0x%02X)\n",b.label,(unsigned)held_key);
     }else if(b.key==0x08){
      std::fprintf(stderr,"SAFE GAME TEST: MAIN drag ignored.\n");
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
  // Throttle EPDC submissions: avoid ~5.5 updates/second seen in reboot log.
  if(now-last_frame>=1100){
   if(wqx::CopyLcdBuffer(lcd) && (!was_drawn||memcmp(lcd,previous,1600)!=0)){
    if(was_drawn)lcd_draw_diff(fb,previous,lcd);
    else lcd_draw(fb,lcd);
    memcpy(previous,lcd,1600);was_drawn=true;
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
 std::fprintf(stderr,"SAFE GAME TEST exit: reason=%s elapsed_ms=%llu (never saving flash/state)\n",
  interrupted?"EXIT/signal":"30-second timeout",(unsigned long long)(clock_ms()-started));
 touch.close_touch();
 std::memcpy(old_visible,old_frame.data(),display_bytes);
 const bool screen_restored=fb.refresh(0,0,fb.w,fb.h,true);
 std::fprintf(stderr,"SAFE GAME TEST framebuffer restore: %s\n",screen_restored?"OK":"FAILED");
 fb.close_fb();return screen_restored?0:12;
}
