#include "world.h"

#include <cstddef>

namespace golden {
namespace {
using Map = char[town_height][town_width + 1];

constexpr Map west = {
    "ttttt..tttttt...ttt",
    "t....t......tt....t",
    "t..ss..ss.........t",
    "t..ss.....tt......t",
    "t......t..........t",
    "t==================",
    "t......t..........t",
    "t...ss.....wwww...t",
    "t..........wwww...t",
    "tt...t......ww...tt",
    "ttttttttttttttttttt",
};
constexpr Map center = {
    "ttttttttttttttttttt",
    "t..####.....####..t",
    "t..####.....####..t",
    "t...==.......==...t",
    "t..t==..===..==.t.t",
    "===================",
    "t..t....===.......t",
    "t.........wwwww...t",
    "t..tt.....wwwww...t",
    "t.....tt...www....t",
    "ttttttttttttttttttt",
};
constexpr Map east = {
    "ttttttttttttttttttt",
    "t.....tt..........t",
    "t..####....sss....t",
    "t..#..#....sss....t",
    "t..####.....s.....t",
    "==================t",
    "t.................t",
    "t....s....####....t",
    "t..sss....#..#....t",
    "t....t....####....t",
    "ttttttttttttttttttt",
};
constexpr Map street = {
    "###################",
    "#..###.......###..#",
    "#..###.......###..#",
    "#..###.......###..#",
    "#.===.........===.#",
    "#=================#",
    "#.===.........===.#",
    "#..###.......###..#",
    "#..###.......###..#",
    "#.................#",
    "###################",
};
constexpr Map room = {
    "###################",
    "#.................#",
    "#..##...===...##..#",
    "#.................#",
    "#.................#",
    "#=================#",
    "#.................#",
    "#..##.........##..#",
    "#..##.........##..#",
    "#.................#",
    "###################",
};
constexpr Map hills = {
    "ttttttttttttttttttt",
    "tsss......ss......t",
    "tsss..tt.....ss...t",
    "t..s..tt..........t",
    "t......t...t..s...t",
    "t=================t",
    "t...ss...t.t......t",
    "t...ss.....tt.....t",
    "t..tt..........ss.t",
    "t..............ss.t",
    "ttttttttttttttttttt",
};
constexpr Map shore = {
    "ttttttttttttttttttt",
    "twwwww.......wwww.t",
    "twwww..ss....wwww.t",
    "twww.........wwww.t",
    "tww....s......www.t",
    "t=================t",
    "twww...s..........t",
    "twww........sss...t",
    "twwww.....wwwww...t",
    "twwww.....wwwww...t",
    "ttttttttttttttttttt",
};
constexpr Map lair = {
    "###################",
    "#ss.......s.....ss#",
    "#..##..##...##....#",
    "#..##..##...##..#.#",
    "#...............#.#",
    "#=================#",
    "#.#...............#",
    "#.#..##...##..##..#",
    "#....##...##..##..#",
    "#ss.....s.......ss#",
    "###################",
};

using A = Area;
using D = Direction;
using R = Role;
struct District {
    District(const char* label, const Map* map, const Residents& roster, Sect school = Sect::None)
        : name(label), ground(map), people(roster), sect(school) {}
    const char* name;
    const Map* ground;
    Residents people;
    Sect sect;
};

const District districts[] = {
    {u8"平安镇西", &west, {{{{14,4},u8"石料管事",R::StoneKeeper},{{5,6},u8"钓叟",R::Fisher}}}},
    {u8"平安镇中", &center, {{{{9,4},u8"村长",R::Elder},{{7,6},u8"书童",R::Guide}}}},
    {u8"平安镇东", &east, {{{{4,6},u8"工地管事",R::Foreman},{{14,6},u8"恶人",R::Bandit}}}},
    {u8"商业街", &street, {{{{7,4},u8"杂货贩",R::Vendor},{{12,6},u8"兵器商",R::Vendor}}}},
    {u8"家中", &room, {{{{8,4},u8"家中床榻",R::Keeper}}}},
    {u8"客栈", &room, {{{{8,4},u8"店小二",R::Vendor},{{13,4},u8"独行大侠",R::Trainer}}}},
    {u8"衙门", &room, {{{{9,4},u8"捕快",R::Guard}}}},
    {u8"药铺", &room, {{{{9,4},u8"平一指",R::Healer}}}},
    {u8"武馆", &room, {{{{9,4},u8"武馆师傅",R::Trainer}}}},
    {u8"商家堡", &street, {{{{9,4},u8"商剑鸣",R::Master}}}, Sect::Bagua},
    {u8"玉女峰", &hills, {{{{9,4},u8"李青照",R::Master}}}, Sect::Flower},
    {u8"五指山", &hills, {{{{9,4},u8"红莲教教主",R::Master}}}, Sect::Lotus},
    {u8"冰火岛", &shore, {{{{9,4},u8"伊贺上忍",R::Master}}}, Sect::Iga},
    {u8"武当山", &hills, {{{{9,4},u8"清虚道长",R::Master}}}, Sect::Taiji},
    {u8"大雪山", &hills, {{{{9,4},u8"白瑞德",R::Master}}}, Sect::Snow},
    {u8"黑森林", &west, {{{{9,4},u8"娜可露",R::Master}}}, Sect::Beast},
    {u8"灵心观", &street, {{{{9,4},u8"茅山道长",R::Master}}}, Sect::Maoshan},
    {u8"铸剑谷", &hills, {{{{9,4},u8"铸剑师",R::Smith}}}},
    {u8"阎罗府", &lair, {{{{9,4},u8"阎罗王",R::Judge}}}},
    {u8"桃花源", &shore, {{{{9,4},u8"老管家",R::Keeper}}}},
    {u8"青龙坛", &lair, {{{{9,4},u8"青龙坛主",R::AltarLord}}}},
    {u8"地罡坛", &lair, {{{{9,4},u8"地罡坛主",R::AltarLord}}}},
    {u8"朱雀坛", &lair, {{{{9,4},u8"朱雀坛主",R::AltarLord}}}},
    {u8"山岚坛", &lair, {{{{9,4},u8"山岚坛主",R::AltarLord}}}},
    {u8"玄武坛", &lair, {{{{9,4},u8"玄武坛主",R::AltarLord}}}},
    {u8"紫煞坛", &lair, {{{{9,4},u8"紫煞坛主",R::AltarLord}}}},
    {u8"天徽坛", &lair, {{{{9,4},u8"天徽坛主",R::AltarLord}}}},
    {u8"暗黑坛", &lair, {{{{9,4},u8"总瓢把子",R::AltarLord}}}},
};
static_assert(sizeof(districts) / sizeof(districts[0]) == static_cast<std::size_t>(A::Count),
              "One district is required for every area");

constexpr Passage links[] = {
    {A::Center,D::Left,A::West,D::Right}, {A::Center,D::Right,A::East,D::Left},
    {A::Center,D::Up,A::Market,D::Down}, {A::Center,D::Down,A::Home,D::Up},
    {A::West,D::Left,A::Snow,D::Right}, {A::West,D::Up,A::Jade,D::Down},
    {A::West,D::Down,A::Forest,D::Up}, {A::East,D::Right,A::Shang,D::Left},
    {A::East,D::Up,A::Wudang,D::Down}, {A::East,D::Down,A::Ice,D::Up},
    {A::Market,D::Up,A::Court,D::Down}, {A::Market,D::Left,A::Inn,D::Right},
    {A::Market,D::Right,A::Clinic,D::Left}, {A::Home,D::Down,A::Dojo,D::Up},
    {A::Snow,D::Left,A::FiveFinger,D::Right}, {A::Snow,D::Up,A::Spirit,D::Down},
    {A::Shang,D::Right,A::Forge,D::Left}, {A::Jade,D::Left,A::Peach,D::Right},
    {A::Shang,D::Up,A::Green,D::Down}, {A::FiveFinger,D::Up,A::Earth,D::Down},
    {A::Ice,D::Down,A::Scarlet,D::Up}, {A::Wudang,D::Up,A::Mountain,D::Down},
    {A::Snow,D::Down,A::Tortoise,D::Up}, {A::Forest,D::Left,A::Purple,D::Right},
    {A::Spirit,D::Up,A::Celestial,D::Down}, {A::Jade,D::Up,A::Dark,D::Down},
};

constexpr Opponent robber{u8"恶人", 30, 10, 1, 7, 160, 20, 22};
constexpr Opponent opponents[] = {
    {u8"青龙坛主", 86, 24, 8, 12, 650, 40, 70},
    {u8"地罡坛主", 105, 29, 10, 14, 900, 55, 85},
    {u8"朱雀坛主", 125, 34, 13, 16, 1200, 65, 100},
    {u8"山岚坛主", 145, 39, 15, 17, 1400, 75, 115},
    {u8"玄武坛主", 170, 44, 18, 18, 1800, 85, 130},
    {u8"紫煞坛主", 195, 51, 21, 19, 2100, 100, 145},
    {u8"天徽坛主", 220, 57, 24, 20, 2600, 120, 160},
    {u8"总瓢把子", 255, 63, 28, 22, 4000, 150, 200},
};

const District& district(Area area) { return districts[static_cast<std::size_t>(area)]; }
}

Point offset(Point point, Direction direction) {
    switch (direction) {
    case D::Up: --point.y; break;
    case D::Down: ++point.y; break;
    case D::Left: --point.x; break;
    case D::Right: ++point.x; break;
    }
    return point;
}

Point gate(Direction direction) {
    switch (direction) {
    case D::Up: return {9, 0};
    case D::Down: return {9, town_height - 1};
    case D::Left: return {0, 5};
    case D::Right: return {town_width - 1, 5};
    }
    return {9, 5};
}

const char* location_name(Area area) { return district(area).name; }
const Residents& residents(Area area) { return district(area).people; }
Sect local_sect(Area area) { return district(area).sect; }

const Resident* at(Area area, Point point) {
    for (const auto& resident : residents(area))
        if (resident.role != R::None && resident.position == point) return &resident;
    return nullptr;
}

const Passage* passage(Area area, Direction direction) {
    for (const auto& link : links)
        if ((link.from == area && link.leave == direction) || (link.to == area && link.arrive == direction))
            return &link;
    return nullptr;
}

Tile terrain(Area area, Point p) {
    if (p.x < 0 || p.x >= town_width || p.y < 0 || p.y >= town_height) return Tile::Wall;
    for (auto direction : {D::Up,D::Down,D::Left,D::Right})
        if (p == gate(direction) && passage(area, direction)) return Tile::Gate;
    if ((p.x == 9 && p.y > 0 && p.y < town_height - 1) ||
        (p.y == 5 && p.x > 0 && p.x < town_width - 1)) return Tile::Road;
    switch ((*district(area).ground)[p.y][p.x]) {
    case '=': return Tile::Road;
    case '#': return Tile::Wall;
    case 't': return Tile::Tree;
    case 's': return Tile::Stones;
    case 'w': return Tile::Water;
    default: return Tile::Grass;
    }
}

bool walkable(Area area, Point point) {
    const auto tile = terrain(area, point);
    return (tile == Tile::Grass || tile == Tile::Road || tile == Tile::Gate) && !at(area, point);
}

int altar_number(Area area) {
    return area >= A::Green && area <= A::Dark ? static_cast<int>(area) - static_cast<int>(A::Green) + 1 : 0;
}

Area altar_area(int number) {
    return static_cast<A>(static_cast<int>(A::Green) + number - 1);
}

bool may_enter(Area area, int progress) { return altar_number(area) <= progress; }

const Opponent& opponent(Role role, Area area) {
    if (role == R::AltarLord && altar_number(area) > 0) return opponents[altar_number(area) - 1];
    return robber;
}
}
