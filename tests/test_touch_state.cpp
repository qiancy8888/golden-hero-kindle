#include "touch.h"
#include <cassert>
#include <cstdio>
#include <linux/input.h>

static bool send(TouchInput& touch,unsigned short type,unsigned short code,int value,TouchPoint& point){
 input_event ev{};ev.type=type;ev.code=code;ev.value=value;
 return touch.process_event(ev,758,1024,point);
}
int main(){
 TouchInput touch;
 touch.xi.minimum=0;touch.xi.maximum=758;
 touch.yi.minimum=0;touch.yi.maximum=1024;
 TouchPoint point;
 assert(!send(touch,EV_ABS,ABS_MT_TRACKING_ID,1,point));
 assert(!send(touch,EV_ABS,ABS_MT_POSITION_X,39,point));
 assert(!send(touch,EV_ABS,ABS_MT_POSITION_Y,47,point));
 assert(!send(touch,EV_KEY,BTN_TOUCH,1,point));
 assert(!send(touch,EV_KEY,BTN_TOOL_FINGER,1,point));
 assert(send(touch,EV_SYN,SYN_REPORT,0,point));
 assert(point.active&&point.pressed&&!point.released&&point.valid);
 assert(point.x==39&&point.y==47);
 assert(!send(touch,EV_ABS,ABS_MT_POSITION_X,708,point));
 assert(!send(touch,EV_ABS,ABS_MT_POSITION_Y,54,point));
 assert(send(touch,EV_SYN,SYN_REPORT,0,point));
 assert(point.active&&!point.pressed&&!point.released&&point.moved);
 assert(point.x>=706&&point.x<=708&&point.y==54);
 assert(!send(touch,EV_SYN,SYN_REPORT,0,point));
 assert(!point.moved);
 assert(!send(touch,EV_ABS,ABS_MT_TRACKING_ID,-1,point));
 assert(!send(touch,EV_KEY,BTN_TOUCH,0,point));
 assert(!send(touch,EV_KEY,BTN_TOOL_FINGER,0,point));
 assert(send(touch,EV_SYN,SYN_REPORT,0,point));
 assert(!point.active&&!point.pressed&&point.released&&point.valid);
 assert(point.x>=706&&point.x<=708&&point.y==54);
 assert(touch.tap_count==1);

 TouchInput fallback;
 fallback.xi.minimum=0;fallback.xi.maximum=758;
 fallback.yi.minimum=0;fallback.yi.maximum=1024;
 assert(!send(fallback,EV_ABS,ABS_MT_POSITION_X,380,point));
 assert(!send(fallback,EV_ABS,ABS_MT_POSITION_Y,512,point));
 assert(!send(fallback,EV_KEY,BTN_TOUCH,1,point));
 assert(send(fallback,EV_SYN,SYN_REPORT,0,point));
 assert(point.pressed&&point.active);
 assert(point.x>=379&&point.x<=380&&point.y==512);
 std::puts("Touch state tests passed: press / move / release / coordinate mapping.");
 return 0;
}
