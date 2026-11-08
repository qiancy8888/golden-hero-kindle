#pragma once
#include <linux/input.h>
#include <string>

struct TouchPoint {
 bool active=false;
 bool pressed=false;
 bool released=false;
 bool moved=false;
 bool valid=false;
 int x=-1,y=-1;
};

struct TouchInput {
 int fd=-1;
 int raw_x=-1, raw_y=-1;
 bool pressed=false;
 bool reported_pressed=false;
 bool multi_touch=false;
 bool trace_events=false;
 unsigned long event_count=0, abs_count=0, key_count=0, syn_count=0, tap_count=0;
 bool valid_x=false, valid_y=false;
 input_absinfo xi{}, yi{};
 bool swap_xy=false, flip_x=false, flip_y=false;
 int last_x=-1,last_y=-1;
 std::string path, name;

 bool open_touch(std::string& error);
 void close_touch();
 bool process_event(const input_event& ev,int width,int height,TouchPoint& point);
 bool poll_point(int width,int height,TouchPoint& point);
 bool poll_tap(int width,int height,int& x,int& y);
};
