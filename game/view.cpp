#include "view.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace golden {
namespace {
struct Glyph {
    uint16_t code;
    uint8_t width;
    uint16_t rows[16];
};

constexpr Glyph glyphs[] = {
#include "opening_glyphs.inc"
};

struct Button {
    Rect bounds;
    Action action;
    const char* text;
};

constexpr Button buttons[] = {
    {{68,  368, 47, 43}, Action::Up, u8"上"},
    {{17,  418, 47, 43}, Action::Left, u8"左"},
    {{68,  418, 47, 43}, Action::Down, u8"下"},
    {{119, 418, 47, 43}, Action::Right, u8"右"},
    {{188, 368, 171, 49}, Action::Confirm, ""},
    {{188, 425, 82, 36}, Action::Back, u8"返回"},
    {{277, 425, 82, 36}, Action::Exit, u8"退出"},
};

void dot(Frame& frame, int x, int y, bool black) {
    if (x < 0 || x >= view_width || y < 0 || y >= view_height) return;
    uint8_t& byte = frame[y * view_stride + x / 8];
    const auto bit = static_cast<uint8_t>(0x80 >> (x % 8));
    if (black) byte |= bit;
    else byte &= static_cast<uint8_t>(~bit);
}

void fill(Frame& frame, Rect rect, bool black = true) {
    for (int y = rect.y; y < rect.y + rect.h; ++y)
        for (int x = rect.x; x < rect.x + rect.w; ++x)
            dot(frame, x, y, black);
}

void outline(Frame& frame, Rect rect, bool black = true) {
    fill(frame, {rect.x, rect.y, rect.w, 1}, black);
    fill(frame, {rect.x, rect.y + rect.h - 1, rect.w, 1}, black);
    fill(frame, {rect.x, rect.y, 1, rect.h}, black);
    fill(frame, {rect.x + rect.w - 1, rect.y, 1, rect.h}, black);
}

const Glyph& next_glyph(const char*& utf8) {
    auto code = static_cast<uint16_t>(static_cast<unsigned char>(*utf8++));
    if ((code & 0xe0) == 0xc0) {
        code = ((code & 0x1f) << 6) | (static_cast<unsigned char>(*utf8++) & 0x3f);
    } else if ((code & 0xf0) == 0xe0) {
        const auto second = static_cast<unsigned char>(*utf8++);
        const auto third = static_cast<unsigned char>(*utf8++);
        code = ((code & 0x0f) << 12) | ((second & 0x3f) << 6) | (third & 0x3f);
    }
    const auto it = std::lower_bound(std::begin(glyphs), std::end(glyphs), code,
        [](const Glyph& glyph, uint16_t wanted) { return glyph.code < wanted; });
    return it != std::end(glyphs) && it->code == code ? *it : glyphs['?' - ' '];
}

int text_width(const char* text, int scale) {
    int width = 0;
    while (*text && *text != '\n') width += next_glyph(text).width * scale;
    return width;
}

void print(Frame& frame, int x, int y, const char* text, int scale = 1, bool black = true) {
    const int left = x;
    while (*text) {
        if (*text == '\n') {
            ++text;
            x = left;
            y += 20 * scale;
            continue;
        }
        const Glyph& glyph = next_glyph(text);
        for (int row = 0; row < 16; ++row)
            for (int col = 0; col < glyph.width; ++col)
                if (glyph.rows[row] & (1u << (glyph.width - 1 - col)))
                    fill(frame, {x + col * scale, y + row * scale, scale, scale}, black);
        x += glyph.width * scale;
    }
}

void centered(Frame& frame, Rect box, const char* text, int scale = 1, bool black = true) {
    print(frame, box.x + (box.w - text_width(text, scale)) / 2,
          box.y + (box.h - 16 * scale) / 2, text, scale, black);
}

using Sprite = uint16_t[16];
constexpr Sprite tree = {
    0x0000, 0x0180, 0x03c0, 0x07e0, 0x03c0, 0x07e0, 0x0ff0, 0x1ff8,
    0x07e0, 0x0ff0, 0x1ff8, 0x3ffc, 0x0180, 0x0180, 0x07e0, 0x0000,
};
constexpr Sprite wall = {
    0xffff, 0x8889, 0x8889, 0xffff, 0xa223, 0xa223, 0xffff, 0x8889,
    0x8889, 0xffff, 0xa223, 0xa223, 0xffff, 0x8889, 0x8889, 0xffff,
};
constexpr Sprite stones = {
    0x0000, 0x0000, 0x0200, 0x0700, 0x0880, 0x1040, 0x17e0, 0x0ffc,
    0x0842, 0x1a22, 0x25fe, 0x4200, 0x41f0, 0x3e00, 0x0000, 0x0000,
};
constexpr Sprite water = {
    0x0000, 0x0000, 0x3030, 0x4848, 0x8787, 0x0000, 0x0000, 0x0c0c,
    0x1212, 0xe1e1, 0x0000, 0x0000, 0x3030, 0x4848, 0x8787, 0x0000,
};
constexpr Sprite person = {
    0x0000, 0x07e0, 0x0ff0, 0x1ff8, 0x07e0, 0x05a0, 0x0420, 0x03c0,
    0x0180, 0x07e0, 0x0b50, 0x0ff0, 0x07e0, 0x03c0, 0x0660, 0x0000,
};
constexpr Sprite player = {
    0x0000, 0x03c0, 0x07e0, 0x0db0, 0x0c30, 0x0a50, 0x0420, 0x03c0,
    0x0180, 0x07e0, 0x0990, 0x0a50, 0x07e0, 0x0240, 0x0660, 0x0000,
};

void sprite(Frame& frame, int x, int y, const Sprite& rows) {
    for (int row = 0; row < 16; ++row)
        for (int col = 0; col < 16; ++col)
            if (rows[row] & (0x8000u >> col)) dot(frame, x + col, y + row, true);
}

void map(Frame& frame, const Town& town, int map_y) {
    constexpr int map_x = (view_width - town_width * 16) / 2;
    outline(frame, {map_x - 2, map_y - 2, town_width * 16 + 4, town_height * 16 + 4});
    for (int row = 0; row < town_height; ++row) {
        for (int col = 0; col < town_width; ++col) {
            const int x = map_x + col * 16;
            const int y = map_y + row * 16;
            switch (town.tile({col, row})) {
            case Tile::Tree: sprite(frame, x, y, tree); break;
            case Tile::Wall: sprite(frame, x, y, wall); break;
            case Tile::Stones: sprite(frame, x, y, stones); break;
            case Tile::Water: sprite(frame, x, y, water); break;
            case Tile::Grass:
                dot(frame, x + 3 + (col * 5 + row) % 9, y + 11, true);
                break;
            case Tile::Road:
                fill(frame, {x + 2, y + 2 + (col % 3) * 11 / 2, 3, 1});
                break;
            }
        }
    }

    const Point npc = town.resident().position;
    sprite(frame, map_x + npc.x * 16, map_y + npc.y * 16, person);
    const Point pos = town.player();
    const int x = map_x + pos.x * 16, y = map_y + pos.y * 16;
    fill(frame, {x, y, 16, 16}, false);
    sprite(frame, x, y, player);
    switch (town.facing()) {
    case Direction::Up: fill(frame, {x + 6, y, 4, 1}); break;
    case Direction::Down: fill(frame, {x + 6, y + 15, 4, 1}); break;
    case Direction::Left: fill(frame, {x, y + 6, 1, 4}); break;
    case Direction::Right: fill(frame, {x + 15, y + 6, 1, 4}); break;
    }
}
}

Action hit_test(Point point) {
    for (const Button& button : buttons) {
        const Rect& r = button.bounds;
        if (point.x >= r.x && point.x < r.x + r.w && point.y >= r.y && point.y < r.y + r.h)
            return button.action;
    }
    return Action::None;
}

bool direction_for(Action action, Direction& direction) {
    switch (action) {
    case Action::Up: direction = Direction::Up; return true;
    case Action::Down: direction = Direction::Down; return true;
    case Action::Left: direction = Direction::Left; return true;
    case Action::Right: direction = Direction::Right; return true;
    default: return false;
    }
}

void render(const Town& town, Frame& frame) {
    frame.fill(0);
    fill(frame, {17, 55, 342, 1});
    fill(frame, {17, 354, 342, 1});
    if (town.page() == Page::Title) {
        centered(frame, {17, 10, 342, 40}, u8"文曲星经典 · Kindle 独立版");
        centered(frame, {17, 63, 342, 65}, u8"黄金英雄坛说", 3);
        map(frame, town, 143);
        centered(frame, {17, 329, 342, 18}, u8"平安镇试玩");
    } else {
        print(frame, 17, 15, town.location(), 2);
        char status[80];
        std::snprintf(status, sizeof(status), u8"实战经验 %d    潜能 %d", town.experience(), town.potential());
        print(frame, 37, 65, status);
        map(frame, town, 90);
        outline(frame, {34, 276, 308, 77});
        if (town.page() == Page::Dialogue) {
            fill(frame, {35, 277, 306, 18});
            print(frame, 44, 278, town.resident().name, 1, false);
            print(frame, 44, 295, town.dialogue());
        } else {
            print(frame, 44, 282, town.journal());
            print(frame, 44, 305, town.nearby() ? town.resident().name : u8"方向键行走，靠近人物可交谈");
        }
    }
    for (const Button& button : buttons) {
        if (button.action == Action::Confirm) {
            fill(frame, button.bounds);
            centered(frame, button.bounds, town.confirm_label(), 2, false);
        } else {
            outline(frame, button.bounds);
            centered(frame, button.bounds, button.text);
        }
    }
    centered(frame, {17, 476, 342, 18}, u8"长按方向可连续移动 · 试玩限时");
}

Rect changed_region(const Frame& before, const Frame& after) {
    int left = view_stride, right = -1, top = view_height, bottom = -1;
    for (int y = 0; y < view_height; ++y) {
        for (int x = 0; x < view_stride; ++x) {
            const int index = y * view_stride + x;
            if (before[index] == after[index]) continue;
            left = std::min(left, x);
            right = std::max(right, x);
            top = std::min(top, y);
            bottom = y;
        }
    }
    if (right < 0) return {0, 0, 0, 0};
    return {left * 8, top, (right - left + 1) * 8, bottom - top + 1};
}
}
