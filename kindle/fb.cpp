#include "fb.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <algorithm>

// MXCFB ABI used by the original Kindle Paperwhite (PW1) firmware.
// Field order and update constants follow NiLuJe/FBInk's mxcfb-kindle.h.
struct MRect { uint32_t top,left,width,height; };
struct AltBuffer { uint32_t phys_addr,width,height; MRect update_region; };
struct MUpdate {
 MRect update_region;
 uint32_t waveform_mode,update_mode,update_marker;
 uint32_t hist_bw_waveform_mode,hist_gray_waveform_mode;
 int32_t temp;
 uint32_t flags;
 AltBuffer alt_buffer_data;
};
static_assert(sizeof(MRect)==16, "Unexpected MXCFB rectangle ABI");
static_assert(sizeof(MUpdate)==72, "Unexpected PW1 MXCFB update ABI");
#ifndef MXCFB_SEND_UPDATE
#define MXCFB_SEND_UPDATE _IOW('F', 0x2E, MUpdate)
#endif
static const uint32_t WAVEFORM_MODE_AUTO = 257;
static const uint32_t UPDATE_MODE_PARTIAL = 0;
static const uint32_t UPDATE_MODE_FULL = 1;
static const int32_t TEMP_USE_AMBIENT = 0x1000;

bool FbScreen::open_fb(std::string& e){
 fd=open("/dev/fb0",O_RDWR|O_CLOEXEC);
 if(fd<0){e="Cannot open /dev/fb0: "+std::string(strerror(errno));return false;}
 fb_fix_screeninfo fix{};
 if(ioctl(fd,FBIOGET_VSCREENINFO,&var)<0 || ioctl(fd,FBIOGET_FSCREENINFO,&fix)<0){
  e="Cannot inspect framebuffer: "+std::string(strerror(errno));close_fb();return false;
 }
 w=(int)var.xres;h=(int)var.yres;depth=(int)var.bits_per_pixel;stride=(int)fix.line_length;bytes=(size_t)fix.smem_len;
 const size_t bytes_per_pixel=(size_t)depth/8;
 const size_t required_rows=(size_t)var.yoffset+(size_t)h;
 const size_t required_cols=((size_t)var.xoffset+(size_t)w)*bytes_per_pixel;
 if(w<200 || h<200 || (depth!=8&&depth!=16&&depth!=32) ||
    stride<=0 || (size_t)stride<required_cols || bytes<(size_t)stride*required_rows){
  e="Unsupported framebuffer geometry/bit depth or virtual bounds";close_fb();return false;
 }
 data=(unsigned char*)mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
 if(data==MAP_FAILED){data=nullptr;e="Framebuffer mmap failed: "+std::string(strerror(errno));close_fb();return false;}
 const bool kpw1_profile=(w==758 && h==1024 && depth==8 && stride==768 &&
                          var.xres_virtual==768 && var.yres_virtual==6144 &&
                          fix.smem_len==4718592 && var.grayscale==1 && var.rotate==3);
 std::fprintf(stderr,"fb %dx%d virtual=%ux%u %dbpp stride=%d smem=%u offset=%u/%u grayscale=%u rotate=%u\n",
  w,h,var.xres_virtual,var.yres_virtual,depth,stride,fix.smem_len,
  var.xoffset,var.yoffset,var.grayscale,var.rotate);
 std::fprintf(stderr,"KPW1 diagnostic profile: %s\n",kpw1_profile?"MATCH":"compatible geometry differs; using ioctl values");
 return true;
}
void FbScreen::close_fb(){
 if(data){munmap(data,bytes);data=nullptr;}
 if(fd>=0){close(fd);fd=-1;}
}
void FbScreen::pixel(int x,int y,uint8_t g){
 if(!data||x<0||x>=w||y<0||y>=h)return;
 const unsigned int yy=(unsigned int)y+var.yoffset,xx=(unsigned int)x+var.xoffset;
 uint8_t* p=data+(size_t)yy*(size_t)stride+(size_t)xx*(size_t)(depth/8);
 if(depth==8){p[0]=(var.grayscale==2)?(uint8_t)(255-g):g;}
 else if(depth==16){
  uint16_t v=(uint16_t)((g>>3)<<11)|((g>>2)<<5)|(g>>3);std::memcpy(p,&v,2);
 }else{
  uint32_t v=(uint32_t)g|((uint32_t)g<<8)|((uint32_t)g<<16)|0xff000000u;std::memcpy(p,&v,4);
 }
}
void FbScreen::rect(int x,int y,int ww,int hh,uint8_t g){
 if(ww<=0||hh<=0)return;
 const int x1=std::max(x,0),x2=std::min(x+ww,w);
 const int y1=std::max(y,0),y2=std::min(y+hh,h);
 for(int j=y1;j<y2;j++)for(int i=x1;i<x2;i++)pixel(i,j,g);
}
void FbScreen::border(int x,int y,int ww,int hh,uint8_t g,int t){
 if(t<=0||ww<=0||hh<=0)return;
 const int th=std::min(t,std::min(ww,hh));
 rect(x,y,ww,th,g);rect(x,y+hh-th,ww,th,g);
 rect(x,y,th,hh,g);rect(x+ww-th,y,th,hh,g);
}
void FbScreen::blit_mono_1bpp(int x,int y,int sw,int sh,const uint8_t* bits,int scale,uint8_t fg,uint8_t bg){
 if(!data||!bits||sw<=0||sh<=0||scale<=0)return;
 const int out_w=sw*scale,out_h=sh*scale;
 if(depth==8&&x>=0&&y>=0&&x+out_w<=w&&y+out_h<=h){
  if(var.grayscale==2){fg=(uint8_t)(255-fg);bg=(uint8_t)(255-bg);}
  std::vector<uint8_t> row((size_t)out_w);
  for(int sy=0;sy<sh;++sy){
   for(int sx=0;sx<sw;++sx){
    const size_t bit=(size_t)sy*(size_t)sw+(size_t)sx;
    const bool on=(bits[bit>>3]&(1u<<(7-(bit&7))))!=0;
    std::memset(row.data()+(size_t)sx*(size_t)scale,on?fg:bg,(size_t)scale);
   }
   for(int dy=0;dy<scale;++dy){
    uint8_t* dst=data+((size_t)(y+sy*scale+dy)+(size_t)var.yoffset)*(size_t)stride+
      ((size_t)x+(size_t)var.xoffset);
    std::memcpy(dst,row.data(),row.size());
   }
  }
  return;
 }
 for(int sy=0;sy<sh;++sy)for(int sx=0;sx<sw;++sx){
  const size_t bit=(size_t)sy*(size_t)sw+(size_t)sx;
  const bool on=(bits[bit>>3]&(1u<<(7-(bit&7))))!=0;
  rect(x+sx*scale,y+sy*scale,scale,scale,on?fg:bg);
 }
}
void FbScreen::blit_mono_1bpp_scaled(int x,int y,int sw,int sh,const uint8_t* bits,int out_w,int out_h,uint8_t fg,uint8_t bg){
 if(!data||!bits||sw<=0||sh<=0||out_w<=0||out_h<=0)return;
 if(x<0||y<0||x+out_w>w||y+out_h>h)return;
 if(depth==8){
  if(var.grayscale==2){fg=(uint8_t)(255-fg);bg=(uint8_t)(255-bg);}
  std::vector<uint8_t> row((size_t)out_w);
  for(int dy=0;dy<out_h;++dy){
   const int sy=(int)((int64_t)dy*sh/out_h);
   for(int dx=0;dx<out_w;++dx){
    const int sx=(int)((int64_t)dx*sw/out_w);
    const size_t bit=(size_t)sy*(size_t)sw+(size_t)sx;
    row[(size_t)dx]=(bits[bit>>3]&(1u<<(7-(bit&7))))?fg:bg;
   }
   uint8_t* dst=data+((size_t)(y+dy)+(size_t)var.yoffset)*(size_t)stride+
     ((size_t)x+(size_t)var.xoffset);
   std::memcpy(dst,row.data(),row.size());
  }
  return;
 }
 for(int dy=0;dy<out_h;++dy)for(int dx=0;dx<out_w;++dx){
  const int sy=(int)((int64_t)dy*sh/out_h),sx=(int)((int64_t)dx*sw/out_w);
  const size_t bit=(size_t)sy*(size_t)sw+(size_t)sx;
  pixel(x+dx,y+dy,(bits[bit>>3]&(1u<<(7-(bit&7))))?fg:bg);
 }
}
bool FbScreen::refresh(int x,int y,int ww,int hh,bool full){
 if(fd<0||ww<=0||hh<=0)return false;
 const int x1=std::max(0,std::min(x,w));
 const int y1=std::max(0,std::min(y,h));
 const int x2=std::max(x1,std::min(x+ww,w));
 const int y2=std::max(y1,std::min(y+hh,h));
 if(x2<=x1||y2<=y1)return false;
 MUpdate upd{};
 upd.update_region={(uint32_t)y1,(uint32_t)x1,(uint32_t)(x2-x1),(uint32_t)(y2-y1)};
 upd.waveform_mode=WAVEFORM_MODE_AUTO;
 upd.update_mode=full?UPDATE_MODE_FULL:UPDATE_MODE_PARTIAL;
 upd.update_marker=0;
 upd.temp=TEMP_USE_AMBIENT;
 const int r=ioctl(fd,MXCFB_SEND_UPDATE,&upd);
 if(r<0)std::fprintf(stderr,"EPDC refresh ioctl failed: %s\n",strerror(errno));
 return r==0;
}
