#pragma once
#include <cstdint>
#include <linux/fb.h>
#include <string>
#include <vector>
enum class FbRefreshResult { Submitted, Busy, Unsupported, Failed };
enum class FbGameUpdate { Mono, Gray4, Clear };
struct FbScreen {
 int fd=-1; int w=0,h=0,depth=0,stride=0;
 size_t bytes=0; unsigned char* data=nullptr;
 fb_var_screeninfo var{};
 bool open_fb(std::string &error);
 void close_fb();
 void pixel(int x,int y,uint8_t gray);
 void rect(int x,int y,int width,int height,uint8_t gray);
 void border(int x,int y,int width,int height,uint8_t gray,int thick=3);
 void blit_mono_1bpp(int x,int y,int source_width,int source_height,const uint8_t* bits,int scale,uint8_t foreground=0,uint8_t background=255);
 void blit_mono_1bpp_scaled(int x,int y,int source_width,int source_height,const uint8_t* bits,int output_width,int output_height,uint8_t foreground=0,uint8_t background=255);
 bool refresh(int x,int y,int width,int height,bool full=false);
 FbRefreshResult refresh_game(int x,int y,int width,int height,FbGameUpdate mode);
private:
 FbRefreshResult submit_refresh(int x,int y,int width,int height,bool full,uint32_t waveform);
};
