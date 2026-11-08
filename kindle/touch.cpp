#include "touch.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cctype>

#ifndef ABS_MT_POSITION_X
#define ABS_MT_POSITION_X 0x35
#define ABS_MT_POSITION_Y 0x36
#endif
#ifndef ABS_MT_TRACKING_ID
#define ABS_MT_TRACKING_ID 0x39
#endif

namespace {
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) ((((x) - 1) / BITS_PER_LONG) + 1)
static bool test_bit(unsigned bit,const unsigned long* array) {
 return (array[bit / BITS_PER_LONG] >> (bit % BITS_PER_LONG)) & 1UL;
}
static bool valid_axis(const input_absinfo& a) { return a.maximum > a.minimum; }
}

bool TouchInput::open_touch(std::string& error){
 int best_fd=-1,best_score=-1;
 input_absinfo best_x{},best_y{};
 bool best_multi=false;
 std::string best_path,best_name;
 for(int i=0;i<32;i++){
  char device_path[64];
  std::snprintf(device_path,sizeof(device_path),"/dev/input/event%d",i);
  int candidate=open(device_path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
  if(candidate<0)continue;
  unsigned long evbits[NBITS(EV_MAX+1)]={};
  unsigned long absbits[NBITS(ABS_MAX+1)]={};
  unsigned long keybits[NBITS(KEY_MAX+1)]={};
  if(ioctl(candidate,EVIOCGBIT(0,sizeof(evbits)),evbits)<0 ||
     !test_bit(EV_ABS,evbits) ||
     ioctl(candidate,EVIOCGBIT(EV_ABS,sizeof(absbits)),absbits)<0){
   close(candidate);continue;
  }
  (void)ioctl(candidate,EVIOCGBIT(EV_KEY,sizeof(keybits)),keybits);
  const bool mt=test_bit(ABS_MT_POSITION_X,absbits)&&test_bit(ABS_MT_POSITION_Y,absbits);
  const bool st=test_bit(ABS_X,absbits)&&test_bit(ABS_Y,absbits);
  const bool has_touch_key=test_bit(BTN_TOUCH,keybits)||test_bit(BTN_TOOL_FINGER,keybits);
  if(!mt&&!st){close(candidate);continue;}
  if(!mt&&!has_touch_key){close(candidate);continue;}
  input_absinfo ax{},ay{};
  const unsigned xcode=mt?ABS_MT_POSITION_X:ABS_X;
  const unsigned ycode=mt?ABS_MT_POSITION_Y:ABS_Y;
  if(ioctl(candidate,EVIOCGABS(xcode),&ax)<0||ioctl(candidate,EVIOCGABS(ycode),&ay)<0||
     !valid_axis(ax)||!valid_axis(ay)){close(candidate);continue;}
  char device_name[256]={};
  if(ioctl(candidate,EVIOCGNAME(sizeof(device_name)),device_name)<0)
   std::strncpy(device_name,"(unknown absolute input device)",sizeof(device_name)-1);
  std::string lower(device_name);
  std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return (char)std::tolower(c);});
  int score=0;
  if(mt)score+=20;
  if(has_touch_key)score+=10;
  if(lower.find("touch")!=std::string::npos)score+=30;
  if(lower.find("screen")!=std::string::npos)score+=10;
  if(lower.find("accelerometer")!=std::string::npos||lower.find("sensor")!=std::string::npos)score-=100;
  if(score>best_score){
   if(best_fd>=0)close(best_fd);
   best_fd=candidate;best_score=score;best_x=ax;best_y=ay;best_multi=mt;
   best_path=device_path;best_name=device_name;
  }else close(candidate);
 }
 if(best_fd<0){error="No touchscreen-like absolute input device found";return false;}
 fd=best_fd;xi=best_x;yi=best_y;multi_touch=best_multi;
 path=best_path;name=best_name;valid_x=valid_y=true;
 raw_x=raw_y=-1;last_x=last_y=-1;pressed=reported_pressed=false;
 std::fprintf(stderr,"touch %s (%s) ranges x=%d..%d y=%d..%d mt=%d score=%d\n",
  path.c_str(),name.c_str(),xi.minimum,xi.maximum,yi.minimum,yi.maximum,multi_touch,best_score);
 return true;
}

void TouchInput::close_touch(){
 if(fd>=0){close(fd);fd=-1;}
 pressed=reported_pressed=false;
 raw_x=raw_y=last_x=last_y=-1;
}

bool TouchInput::process_event(const input_event& ev,int width,int height,TouchPoint& point){
 point=TouchPoint{};
 ++event_count;
 if(ev.type==EV_ABS)++abs_count;
 if(ev.type==EV_KEY)++key_count;
 if(ev.type==EV_SYN)++syn_count;
 if(trace_events&&event_count<=160){
  std::fprintf(stderr,"Touch raw #%lu: type=%u code=%u value=%d pressed=%d raw=(%d,%d)\n",
   event_count,ev.type,ev.code,ev.value,(int)pressed,raw_x,raw_y);
 }
 if(ev.type==EV_ABS){
  if(ev.code==ABS_X||ev.code==ABS_MT_POSITION_X)raw_x=ev.value;
  if(ev.code==ABS_Y||ev.code==ABS_MT_POSITION_Y)raw_y=ev.value;
  if(ev.code==ABS_MT_TRACKING_ID)pressed=(ev.value>=0);
 }
 if(ev.type==EV_KEY&&(ev.code==BTN_TOUCH||ev.code==BTN_TOOL_FINGER))
  pressed=(ev.value!=0);
 if(ev.type!=EV_SYN||ev.code!=SYN_REPORT)return false;

 const bool have_coords=raw_x>=0&&raw_y>=0;
 int mapped_x=last_x,mapped_y=last_y;
 if(have_coords){
  auto norm=[](int v,const input_absinfo& a){
   return std::max(0.0,std::min(1.0,double(v-a.minimum)/double(std::max(1,a.maximum-a.minimum))));
  };
  double nx=norm(raw_x,xi),ny=norm(raw_y,yi);
  if(swap_xy)std::swap(nx,ny);
  if(flip_x)nx=1.0-nx;
  if(flip_y)ny=1.0-ny;
  mapped_x=(int)std::round(nx*(width-1));
  mapped_y=(int)std::round(ny*(height-1));
 }
 point.active=pressed;
 point.pressed=pressed&&!reported_pressed;
 point.released=!pressed&&reported_pressed;
 point.valid=have_coords||(last_x>=0&&last_y>=0);
 point.x=mapped_x;point.y=mapped_y;
 point.moved=pressed&&have_coords&&(mapped_x!=last_x||mapped_y!=last_y);
 if(have_coords){last_x=mapped_x;last_y=mapped_y;}
 reported_pressed=pressed;
 if(point.pressed||point.released||point.moved){
  if(point.released)++tap_count;
  return true;
 }
 return false;
}

bool TouchInput::poll_point(int width,int height,TouchPoint& point){
 if(fd<0)return false;
 input_event ev{};
 for(;;){
  const ssize_t n=read(fd,&ev,sizeof(ev));
  if(n==(ssize_t)sizeof(ev)){
   if(process_event(ev,width,height,point))return true;
   continue;
  }
  if(n<0&&errno==EINTR)continue;
  break;
 }
 return false;
}

bool TouchInput::poll_tap(int width,int height,int& x,int& y){
 TouchPoint point;
 while(poll_point(width,height,point)){
  if(point.released&&point.valid){x=point.x;y=point.y;return true;}
 }
 return false;
}
