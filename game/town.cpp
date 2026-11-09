#include "town.h"

#include <cstdlib>

namespace golden {
namespace {
constexpr int road_y = 5;
constexpr int experience_reward = 40;
constexpr int potential_reward = 20;

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

constexpr Resident stone_keeper{{14, 4}, u8"石料管事"};
constexpr Resident village_head{{9, 4}, u8"村长"};
constexpr Resident foreman{{4, 6}, u8"工地管事"};
}

const Resident& Town::resident() const {
    if (area_ == Area::West) return stone_keeper;
    if (area_ == Area::East) return foreman;
    return village_head;
}

Tile Town::tile(Point point) const {
    if (point.x < 0 || point.x >= town_width || point.y < 0 || point.y >= town_height)
        return Tile::Wall;
    const Map& map = area_ == Area::West ? west : area_ == Area::East ? east : center;
    switch (map[point.y][point.x]) {
    case '=': return Tile::Road;
    case '#': return Tile::Wall;
    case 't': return Tile::Tree;
    case 's': return Tile::Stones;
    case 'w': return Tile::Water;
    default: return Tile::Grass;
    }
}

bool Town::nearby() const {
    const Point npc = resident().position;
    return std::abs(player_.x - npc.x) + std::abs(player_.y - npc.y) == 1;
}

const char* Town::location() const {
    if (area_ == Area::West) return u8"平安镇 · 镇西";
    if (area_ == Area::East) return u8"平安镇 · 镇东";
    return u8"平安镇 · 镇中";
}

const char* Town::journal() const {
    switch (job_) {
    case Job::Available: return u8"去镇西找石料管事";
    case Job::Carrying: return u8"把石料送到镇东工地";
    case Job::Complete: return u8"搬运完成，可以继续逛镇";
    }
    return "";
}

const char* Town::dialogue() const {
    if (area_ == Area::Center) {
        return u8"你是新来的吧？\n"
               u8"沿大路往西走，石料管事缺人。\n"
               u8"领了石料，再送到镇东工地。";
    }
    if (area_ == Area::West) {
        if (job_ == Job::Available)
            return u8"劳驾，把这批石料送到镇东。\n"
                   u8"过了镇中，找工地管事交差。";
        if (job_ == Job::Carrying)
            return u8"石料已经给你了。\n"
                   u8"沿着大路往东走就是工地。";
        return u8"石料送到了？多谢你跑这一趟。";
    }
    if (job_ == Job::Carrying)
        return u8"石料可算送到了。\n来，交给我就能领到奖励。";
    if (job_ == Job::Complete)
        return u8"搬运完成！\n实战经验 +40，潜能 +20。";
    return u8"这里是工地。\n要帮忙的话，先去镇西领石料。";
}

const char* Town::confirm_label() const {
    if (page_ == Page::Title) return u8"进入平安镇";
    if (page_ == Page::Town) return nearby() ? u8"交谈" : u8"确定";
    if (area_ == Area::West && job_ == Job::Available) return u8"领石料";
    if (area_ == Area::East && job_ == Job::Carrying) return u8"交石料";
    return u8"确定";
}

bool Town::move(Direction direction) {
    if (page_ != Page::Town) return false;
    Point next = player_;
    switch (direction) {
    case Direction::Up: --next.y; break;
    case Direction::Down: ++next.y; break;
    case Direction::Left: --next.x; break;
    case Direction::Right: ++next.x; break;
    }
    const bool turned = direction != facing_;
    facing_ = direction;
    if (next.y == road_y) {
        if (next.x < 0 && area_ != Area::West) {
            area_ = area_ == Area::Center ? Area::West : Area::Center;
            player_.x = town_width - 1;
            return true;
        }
        if (next.x >= town_width && area_ != Area::East) {
            area_ = area_ == Area::Center ? Area::East : Area::Center;
            player_.x = 0;
            return true;
        }
    }
    const Point npc = resident().position;
    if (next.x == npc.x && next.y == npc.y) return turned;
    const Tile target = tile(next);
    if (target != Tile::Grass && target != Tile::Road) return turned;
    player_ = next;
    return true;
}

bool Town::confirm() {
    if (page_ == Page::Title) {
        page_ = Page::Town;
        return true;
    }
    if (page_ == Page::Town) {
        if (!nearby()) return false;
        page_ = Page::Dialogue;
        return true;
    }
    if (area_ == Area::East && job_ == Job::Carrying) {
        job_ = Job::Complete;
        experience_ += experience_reward;
        potential_ += potential_reward;
        return true;
    }
    if (area_ == Area::West && job_ == Job::Available) job_ = Job::Carrying;
    page_ = Page::Town;
    return true;
}

bool Town::back() {
    if (page_ != Page::Dialogue) return false;
    page_ = Page::Town;
    return true;
}
}
