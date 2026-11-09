#pragma once

#include "rpg.h"

namespace golden {
constexpr int town_width = 19;
constexpr int town_height = 11;
constexpr uint8_t starting_altar = 1;
constexpr uint8_t completed_altars = altar_count + 1;

enum class Direction { Up, Down, Left, Right };
enum class Area { West, Center, East, Market, Home, Inn, Court, Clinic, Dojo,
                  Shang, Jade, FiveFinger, Ice, Wudang, Snow, Forest, Spirit, Forge, Underworld,
                  Peach, Green, Earth, Scarlet, Mountain, Tortoise, Purple, Celestial, Dark, Count };
enum class Tile { Grass, Road, Wall, Tree, Stones, Water, Gate };

struct Point {
    int x;
    int y;
};

inline bool operator==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(Point a, Point b) { return !(a == b); }

enum class Role { None, Elder, StoneKeeper, Foreman, Guide, Vendor, Guard, Healer, Trainer,
                  Master, Bandit, AltarLord, Smith, Fisher, Keeper, Judge };

struct Resident {
    Point position;
    const char* name;
    Role role;
};

using Residents = std::array<Resident, 4>;

struct Passage {
    Area from;
    Direction leave;
    Area to;
    Direction arrive;
};

Point offset(Point point, Direction direction);
Point gate(Direction direction);
const char* location_name(Area area);
const Residents& residents(Area area);
const Resident* at(Area area, Point point);
Tile terrain(Area area, Point point);
bool walkable(Area area, Point point);
const Passage* passage(Area area, Direction direction);
bool may_enter(Area area, int progress);
int altar_number(Area area);
Area altar_area(int number);
Sect local_sect(Area area);
const Opponent& opponent(Role role, Area area);
}
