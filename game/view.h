#pragma once

#include "town.h"

#include <array>
#include <cstdint>

namespace golden {
constexpr int view_width = 376;
constexpr int view_height = 512;
constexpr int view_stride = view_width / 8;

using Frame = std::array<uint8_t, view_stride * view_height>;

struct Rect {
    int x;
    int y;
    int w;
    int h;
};

enum class Action { None, Up, Down, Left, Right, Confirm, Back, Menu, Exit };

Action hit_test(Point point);
bool direction_for(Action action, Direction& direction);
void render(const Town& town, Frame& frame);
Rect changed_region(const Frame& before, const Frame& after);
}
