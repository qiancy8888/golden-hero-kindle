#include "save.h"
#include "view.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <vector>

using namespace golden;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition); \
        std::exit(1); \
    } \
} while (false)

namespace {
constexpr Direction directions[] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};

template <typename E> int index(E value) { return static_cast<int>(value); }
int index(Point point) { return point.y * town_width + point.x; }

void walk_to(Town& town, Point goal) {
    const int start = index(town.player());
    const int end = index(goal);
    constexpr int size = town_width * town_height;
    std::array<int, size> parents;
    parents.fill(-1);
    std::array<Direction, size> steps{};
    std::queue<Point> pending;
    pending.push(town.player());
    parents[start] = start;
    while (!pending.empty() && parents[end] < 0) {
        const auto current = pending.front();
        pending.pop();
        for (auto direction : directions) {
            const Point next = offset(current, direction);
            if (!walkable(town.area(), next) || parents[index(next)] >= 0) continue;
            parents[index(next)] = index(current);
            steps[index(next)] = direction;
            pending.push(next);
        }
    }
    if (parents[end] < 0)
        std::fprintf(stderr, "No route from (%d,%d) to (%d,%d) in area %d\n",
                     town.player().x, town.player().y, goal.x, goal.y, index(town.area()));
    CHECK(parents[end] >= 0);
    std::vector<Direction> path;
    for (int cell = end; cell != start; cell = parents[cell]) path.push_back(steps[cell]);
    for (auto it = path.rbegin(); it != path.rend(); ++it) CHECK(town.move(*it));
    CHECK(town.player() == goal);
}

void go(Town& town, Area goal) {
    constexpr int size = static_cast<int>(Area::Count);
    const int start = index(town.area());
    const int end = index(goal);
    std::array<int, size> parents;
    parents.fill(-1);
    std::array<Direction, size> steps{};
    std::queue<Area> pending;
    pending.push(town.area());
    parents[start] = start;
    while (!pending.empty() && parents[end] < 0) {
        const auto area = pending.front();
        pending.pop();
        for (auto direction : directions) {
            const auto* link = passage(area, direction);
            if (!link) continue;
            const Area next = link->from == area ? link->to : link->from;
            if (!may_enter(next, town.journey().altar) || parents[index(next)] >= 0) continue;
            parents[index(next)] = index(area);
            steps[index(next)] = direction;
            pending.push(next);
        }
    }
    CHECK(parents[end] >= 0);
    std::vector<Direction> path;
    for (int area = end; area != start; area = parents[area]) path.push_back(steps[area]);
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        walk_to(town, gate(*it));
        CHECK(town.move(*it));
    }
    CHECK(town.area() == goal);
}

void talk(Town& town, Role role) {
    CHECK(town.page() == Page::Town);
    for (const auto& resident : residents(town.area())) {
        if (resident.role != role) continue;
        for (auto direction : directions) {
            const auto next = offset(resident.position, direction);
            if (!walkable(town.area(), next)) continue;
            walk_to(town, next);
            for (auto face : directions)
                if (offset(next, face) == resident.position) town.move(face);
            CHECK(town.nearby() && town.nearby()->role == role);
            CHECK(town.confirm() && town.page() == Page::Actions);
            return;
        }
    }
    CHECK(false);
}

void choose(Town& town, const std::string& label) {
    for (int attempt = 0; attempt < town.menu_size(); ++attempt) {
        if (town.menu_label(town.selection()).compare(0, label.size(), label) == 0) {
            CHECK(town.confirm());
            return;
        }
        CHECK(town.move(Direction::Down));
    }
    std::fprintf(stderr, "Missing menu entry: %s (page=%d)\n", label.c_str(), index(town.page()));
    CHECK(false);
}

void train(Town& town, Skill skill, int level) {
    CHECK(town.menu());
    choose(town, u8"武学修为");
    while (town.hero().skills[index(skill)] < level) {
        const int before = town.hero().skills[index(skill)];
        choose(town, skill_name(skill));
        CHECK(town.hero().skills[index(skill)] == before + 1);
    }
    CHECK(town.back() && town.back());
}

void rest(Town& town) {
    go(town, Area::Home);
    talk(town, Role::Keeper);
    choose(town, u8"歇息调养");
    CHECK(town.hero().health == town.hero().max_health());
    CHECK(town.back());
}

void battle(Town& town) {
    CHECK(town.page() == Page::Battle && !town.can_save());
    for (int turn = 0; turn < 100 && town.page() == Page::Battle; ++turn) {
        if (town.hero().energy >= 8) {
            town.move(Direction::Up);
            while (town.selection() != 1) CHECK(town.move(Direction::Down));
            CHECK(town.confirm());
        } else choose(town, u8"普通攻击");
    }
    CHECK(town.page() != Page::Battle);
    CHECK(town.area() != Area::Underworld);
}

void save_roundtrip(Town& town) {
    const auto bytes = encode_save(town.journey(), 12);
    CHECK(!bytes.empty());
    Journey loaded;
    uint32_t generation = 0;
    CHECK(decode_save(bytes, loaded, generation) == SaveStatus::Loaded && generation == 12);
    CHECK(encode_save(loaded, generation) == bytes);
    auto damaged = bytes;
    damaged.back() ^= 4;
    CHECK(decode_save(damaged, loaded, generation) == SaveStatus::Damaged);
    damaged = bytes;
    damaged[4] = 3;
    CHECK(decode_save(damaged, loaded, generation) == SaveStatus::Unsupported);
    CHECK(town.resume(loaded));
    choose(town, u8"继续江湖");
}

void read_legacy_save() {
    // Version 1 stored five skills and used item ID 11 for an empty slot.
    const SaveBytes original{
+        0x47, 0x48, 0x4b, 0x50, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x09, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0xdc, 0x05, 0x00, 0x00,
        0x2d, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x93, 0x6e, 0x72, 0x9c,
    };
    Journey journey;
    uint32_t generation = 0;
    CHECK(decode_save(original, journey, generation) == SaveStatus::Loaded);
    CHECK(generation == 17 && journey.hero.weapon == Item::WoodSword);
    CHECK(journey.hero.armor == Item::Count && journey.hero.sect == Sect::Taiji);
    CHECK(journey.hero.skills[index(Skill::TaijiFist)] == 3);
    CHECK(journey.hero.skills[index(Skill::Sword)] == 12);
    CHECK(journey.hero.skills[index(Skill::Legacy)] == 0);
    CHECK(encode_save(journey, 18).at(4) == 2);
}
}

int main() {
    read_legacy_save();
    Town town;
    CHECK(town.page() == Page::Title);
    choose(town, u8"新建人物");
    CHECK(town.page() == Page::Create);
    CHECK(town.move(Direction::Right));
    CHECK(town.move(Direction::Down) && town.move(Direction::Right));
    CHECK(town.move(Direction::Down) && town.move(Direction::Left));
    choose(town, u8"进入平安镇");
    CHECK(town.hero().age() == 14 && town.hero().appearance == 1 && town.hero().name == 1);
    CHECK(town.hero().coins == 100 && town.hero().potential == 100);
    CHECK(town.hero().health == 100 && town.hero().energy == 0);
    CHECK(town.hero().attributes[0] == 19 && town.take_save_request());
    CHECK(town.nearby() && town.nearby()->role == Role::Elder);
    CHECK(!town.move(Direction::Up));

    Frame before, after;
    render(town, before);
    CHECK(changed_region(before, before).w == 0);
    CHECK(town.move(Direction::Right));
    render(town, after);
    const Rect moved = changed_region(before, after);
    CHECK(moved.w > 0 && moved.w < view_width && moved.x % 8 == 0);
    CHECK(hit_test({40, 390}) == Action::Menu);
    CHECK(hit_test({90, 386}) == Action::Up);
    CHECK(hit_test({16, 435}) == Action::None);
    CHECK(hit_test({300, 447}) == Action::Exit);

    talk(town, Role::Guide);
    choose(town, u8"打听镇上的消息");
    CHECK(town.hero().amount(Item::WoodSword) == 1);
    CHECK(town.back() && town.menu());
    choose(town, u8"背包与装备");
    choose(town, u8"木剑");
    CHECK(town.hero().weapon == Item::WoodSword);
    CHECK(town.back() && town.back());
    train(town, Skill::Sword, 4);

    for (int i = 0; i < 6; ++i) {
        go(town, Area::Court);
        talk(town, Role::Guard);
        choose(town, u8"领取除恶差事");
        go(town, Area::East);
        talk(town, Role::Bandit);
        choose(town, u8"出手缉拿");
        battle(town);
        CHECK(town.journey().hunt == Hunt::Report);
        go(town, Area::Court);
        talk(town, Role::Guard);
        choose(town, u8"领取差事赏金");
        rest(town);
    }

    go(town, Area::West);
    talk(town, Role::StoneKeeper);
    choose(town, u8"领石料");
    CHECK(town.hero().amount(Item::Stone) == 1 && town.journey().job == Job::Carrying);
    save_roundtrip(town);
    go(town, Area::East);
    talk(town, Role::Foreman);
    const int exp = town.hero().experience;
    choose(town, u8"交石料");
    CHECK(town.hero().amount(Item::Stone) == 0 && town.hero().experience > exp);
    talk(town, Role::Foreman);
    choose(town, u8"结束交谈");
    CHECK(town.hero().amount(Item::Stone) == 0);

    go(town, Area::Market);
    talk(town, Role::Vendor);
    choose(town, u8"买卖物品");
    choose(town, u8"长剑");
    choose(town, u8"布衣");
    CHECK(town.back() && town.back() && town.menu());
    choose(town, u8"背包与装备");
    choose(town, u8"长剑");
    choose(town, u8"布衣");
    CHECK(town.back() && town.back());

    go(town, Area::Wudang);
    talk(town, Role::Master);
    choose(town, u8"拜师入门");
    CHECK(town.hero().sect == Sect::Taiji);
    train(town, Skill::Sword, 33);
    train(town, Skill::Force, 16);
    train(town, Skill::TaijiSword, 2);
    CHECK(town.hero().special_style() == Skill::TaijiSword);
    go(town, Area::Center);
    talk(town, Role::Elder);
    choose(town, u8"领取青龙坛地图");
    CHECK(town.journey().altar == 1);
    CHECK(!may_enter(Area::Earth, town.journey().altar));
    for (int altar = 1; altar <= altar_count; ++altar) {
        rest(town);
        go(town, altar_area(altar));
        talk(town, Role::AltarLord);
        choose(town, u8"挑战坛主");
        battle(town);
        CHECK(town.journey().altar == altar + 1);
        CHECK(town.hero().sect == Sect::Taiji);
        if (altar < altar_count) save_roundtrip(town);
    }
    CHECK(town.page() == Page::Ending && town.journey().altar == completed_altars);
    CHECK(town.can_save());
    save_roundtrip(town);
    CHECK(town.menu());
    choose(town, u8"查看结局");
    CHECK(town.page() == Page::Ending);
    std::puts("Standalone campaign checks passed.");
}
