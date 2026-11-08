#include "fb.h"
#include "touch.h"
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>

// No emulator and no ROM files are accessed by this on-device smoke test.
// Preserve the visible framebuffer and restore it on normal/signal/timeout exit.
namespace {
volatile sig_atomic_t stopped=0;
void stop_test(int) { stopped=1; }
uint64_t monotonic_ms() {
    struct timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000;
}
const char* pattern(char c) {
    switch(c) {
        case '0': return "011101001110101101011001101110";
        case '1': return "001000110000100001000010001110";
        case 'A': return "011101000110001111111000110001";
        case 'C': return "011111000010000100001000001111";
        case 'E': return "111111000011110100001000011111";
        case 'H': return "100011000110001111111000110001";
        case 'I': return "111110010000100001000010011111";
        case 'K': return "100011001010100110001010010001";
        case 'N': return "100011100110101100111000110001";
        case 'O': return "011101000110001100011000101110";
        case 'P': return "111101000110001111101000010000";
        case 'T': return "111110010000100001000010000100";
        case 'U': return "100011000110001100011000101110";
        case 'W': return "100011000110001101011101110001";
        case 'X': return "100011000101010001000101010001";
        case 'Y': return "100011000101010001000010000100";
        case ' ': return "000000000000000000000000000000";
        default: return "000000000000000000000000000000";
    }
}
void print_text(FbScreen& fb,int x,int y,const char* s,int scale){
    for(;*s;s++,x+=6*scale) {
        const char* b=pattern(*s);
        for(int yy=0;yy<6;yy++)for(int xx=0;xx<5;xx++)
            if(b[yy*5+xx]=='1') fb.rect(x+xx*scale,y+yy*scale,scale,scale,0);
    }
}
}

int run_display_test() {
    std::fprintf(stderr,"Golden Hero KPW1: ROM-free display/touch test (max 40 seconds).\n");
    std::string error;
    FbScreen fb;
    if(!fb.open_fb(error)) {
        std::fprintf(stderr,"FrameBuffer failure: %s\n",error.c_str());
        return 10;
    }
    // Safe because open_fb verified smem_len, line_length and visible offsets.
    const size_t region=(size_t)fb.stride*(size_t)fb.h;
    unsigned char* start=fb.data+(size_t)fb.stride*(size_t)fb.var.yoffset;
    const std::vector<unsigned char> backup(start,start+region);
    TouchInput touch;
    const bool touch_ready=touch.open_touch(error);
    if(!touch_ready)std::fprintf(stderr,"Touch not accessible: %s; automatic exit still works.\n",error.c_str());
    fb.rect(0,0,fb.w,fb.h,255);
    fb.rect(0,0,fb.w,7,0); // physical top edge marker
    print_text(fb,fb.w/2-3*6*5,35,"TOP",5);
    print_text(fb,fb.w/2-4*6*4,115,"KPW1",4);
    fb.border(45,205,fb.w-90,420,0,5);
    for(int i=0;i<8;i++){
        int x=55+i*(fb.w-110)/8;
        int gray=std::min(255,i*34);
        fb.rect(x,235,(fb.w-110)/8,350,(uint8_t)gray);
    }
    print_text(fb,fb.w/2-5*6*4,650,"TOUCH",4);
    const int ex=140, ey=fb.h-170, ew=fb.w-280, eh=105;
    fb.border(ex,ey,ew,eh,0,6);
    print_text(fb,ex+(ew-4*6*5)/2,ey+(eh-6*5)/2,"EXIT",5);
    if(!fb.refresh(0,0,fb.w,fb.h,true)){
        std::fprintf(stderr,"Initial MXCFB refresh failed.\n");
    }
    const uint64_t starttime=monotonic_ms();
    int rc=0;
    stopped=0;
    std::signal(SIGINT,stop_test);
    std::signal(SIGTERM,stop_test);
    while(!stopped && monotonic_ms()-starttime<40000) {
        int x=0,y=0;
        if(touch_ready && touch.poll_tap(fb.w,fb.h,x,y)){
            std::fprintf(stderr,"Touch x=%d y=%d\n",x,y);
            if(x>=ex&&x<ex+ew&&y>=ey&&y<ey+eh)break;
            fb.rect(x-14,y-14,29,29,0);
            fb.refresh(std::max(0,x-14),std::max(0,y-14),29,29,false);
        }
        usleep(35000);
    }
    std::memcpy(start,backup.data(),region);
    if(!fb.refresh(0,0,fb.w,fb.h,true)){
        std::fprintf(stderr,"Warning: display restore refresh failed.\n");
        rc=11;
    }
    if(touch_ready)touch.close_touch();
    fb.close_fb();
    std::fprintf(stderr,"Display test finished. Previous framebuffer content restored.\n");
    return rc;
}
