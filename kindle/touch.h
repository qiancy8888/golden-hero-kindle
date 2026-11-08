#pragma once
#include <linux/input.h>
#include <string>

struct TouchInput {
 int fd=-1;
 int raw_x=-1, raw_y=-1;
 bool pressed=false;
 bool multi_touch=false;
 bool valid_x=false, valid_y=false;
 input_absinfo xi{}, yi{};
 bool swap_xy=false, flip_x=false, flip_y=false;
 std::string path, name;

 bool open_touch(std::string& error);
 void close_touch();
 // Returns one completed tap in framebuffer coordinates.
 bool poll_tap(int width,int height,int &x,int &y);
};
