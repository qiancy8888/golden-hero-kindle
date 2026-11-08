#include "fb.h"
#include "touch.h"
#include <algorithm>
#include <cstdint>
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
    std::fprintf(stderr,"Golden Hero KPW1: ROM-free display/touch diagnostic (max 75 seconds).\n");
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
    touch.trace_events=true;
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
    const bool initial_refresh=fb.refresh(0,0,fb.w,fb.h,true);
    std::fprintf(stderr,"Initial full-screen MXCFB refresh: %s\n",initial_refresh?"OK":"FAILED");
    const uint64_t starttime=monotonic_ms();

    struct PixelProbe {int x,y;const char* label;uint8_t expected;};
    const auto pixel_at=[&fb](int x,int y)->uint8_t{
        const size_t addr=((size_t)y+(size_t)fb.var.yoffset)*(size_t)fb.stride+
                          ((size_t)x+(size_t)fb.var.xoffset)*(size_t)(fb.depth/8);
        return fb.data[addr];
    };
    PixelProbe probes[]={
      {3,3,"top-border",0},
      {fb.w/2,fb.h-4,"bottom-white",255},
      {60,260,"gray-0",0},
      {145,260,"gray-1",0},
      {310,260,"gray-3",0},
      {fb.w/2,870,"exit-region",0}
    };
    for(auto& p:probes)p.expected=pixel_at(p.x,p.y);
    std::fprintf(stderr,"Drawn framebuffer samples: ");
    for(const auto& p:probes)std::fprintf(stderr,"%s=%u ",p.label,(unsigned)p.expected);
    std::fprintf(stderr,"\n");
    // Re-read the mmap every 2.5 seconds. If the pixels change, another
    // process may be repainting the framebuffer (e.g. Kindle's X11 UI).
    uint64_t last_probe=starttime;
    unsigned sample_changes=0;
    unsigned samples_checked=0;
    bool exited_by_touch=false;
    int rc=0;
    stopped=0;
    std::signal(SIGINT,stop_test);
    std::signal(SIGTERM,stop_test);
    while(!stopped && monotonic_ms()-starttime<75000) {
        int x=0,y=0;
        if(touch_ready && touch.poll_tap(fb.w,fb.h,x,y)){
            std::fprintf(stderr,"Touch x=%d y=%d\n",x,y);
            if(x>=ex&&x<ex+ew&&y>=ey&&y<ey+eh){exited_by_touch=true;break;}
            fb.rect(x-14,y-14,29,29,0);
            fb.refresh(std::max(0,x-14),std::max(0,y-14),29,29,false);
        }
        const uint64_t now=monotonic_ms();
        if(now-last_probe>=2500){
            ++samples_checked;
            unsigned mismatches=0;
            for(const auto& p:probes){
                const uint8_t current=pixel_at(p.x,p.y);
                if(current!=p.expected){
                    ++mismatches;
                    std::fprintf(stderr,"Framebuffer sample changed: %s old=%u new=%u\n",
                                 p.label,(unsigned)p.expected,(unsigned)current);
                }
            }
            if(mismatches)++sample_changes;
            std::fprintf(stderr,"Elapsed=%llu ms, mismatched probes=%u/%zu, input events=%lu (ABS=%lu KEY=%lu SYN=%lu), completed taps=%lu\n",
                         (unsigned long long)(now-starttime),mismatches,sizeof(probes)/sizeof(probes[0]),
                         touch.event_count,touch.abs_count,touch.key_count,touch.syn_count,touch.tap_count);
            last_probe=now;
        }
        usleep(35000);
    }
    std::fprintf(stderr,"End reason=%s, elapsed=%llu ms, event_count=%lu abs=%lu key=%lu syn=%lu taps=%lu, pixel sample change intervals=%u/%u\n",
                 stopped?"signal":exited_by_touch?"EXIT-tap":"timeout",
                 (unsigned long long)(monotonic_ms()-starttime),
                 touch.event_count,touch.abs_count,touch.key_count,touch.syn_count,touch.tap_count,
                 sample_changes,samples_checked);
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
