#include "rpg.h"

#include <algorithm>
#include <numeric>

namespace golden {
namespace {
template <typename E> constexpr std::size_t index(E item) {
    return static_cast<std::size_t>(item);
}
constexpr ItemDef items[] = {
    {u8"包子", 4, 0, 0}, {u8"清水", 2, 0, 0}, {u8"金创药", 12, 0, 0},
    {u8"木剑", 15, 5, 0}, {u8"长剑", 55, 13, 0}, {u8"布衣", 18, 0, 5},
    {u8"石料", 0, 0, 0}, {u8"钓竿", 24, 0, 0}, {u8"鲤鱼", 5, 0, 0},
    {u8"铁矿", 0, 0, 0}, {u8"自铸剑", 0, 28, 0},
};
constexpr SectDef sects[] = {
    {u8"无门无派", u8"门派武学", ""}, {u8"太极门", u8"太极拳", u8"清虚道长"},
    {u8"八卦门", u8"八卦掌", u8"商剑鸣"}, {u8"雪山派", u8"雪山剑法", u8"白瑞德"},
    {u8"花间派", u8"一剪梅花手", u8"李青照"}, {u8"红莲教", u8"红莲掌", u8"张无忌"},
    {u8"伊贺谷", u8"忍术", u8"伊贺上忍"}, {u8"兽王派", u8"猛虎拳", u8"娜可露"},
    {u8"茅山派", u8"天师正道", u8"茅山道长"},
};
constexpr const char* names[] = {u8"无名", u8"云舟", u8"青岚", u8"飞雪", u8"长风", u8"明月"};
constexpr const char* appearances[] = {u8"少侠", u8"少女", u8"少年"};
constexpr const char* attributes[] = {u8"膂力", u8"敏捷", u8"悟性", u8"根骨"};
constexpr const char* skills[] = {u8"基本拳脚", u8"基本剑法", u8"基本轻功", u8"基本内功", u8"门派武学"};

int roll(uint32_t& state, int maximum) {
    state = state * 1664525u + 1013904223u;
    return static_cast<int>((state >> 8) % static_cast<uint32_t>(maximum));
}
}

const char* hero_name(int name) { return names[name]; }
const char* appearance_name(int appearance) { return appearances[appearance]; }
const char* attribute_name(Attribute attribute) { return attributes[index(attribute)]; }
const char* skill_name(Skill skill) { return skills[index(skill)]; }
const ItemDef& item_def(Item item) { return items[index(item)]; }
const SectDef& sect_def(Sect sect) { return sects[index(sect)]; }

int Hero::max_health() const {
    return 60 + 2 * attributes[index(Attribute::Constitution)] + 3 * skills[index(Skill::Force)];
}

int Hero::max_energy() const {
    return 4 * skills[index(Skill::Force)];
}

int Hero::attack() const {
    const bool armed = weapon != Item::Count;
    const int base_skill = skills[index(armed ? Skill::Sword : Skill::Fist)];
    return 5 + attributes[index(Attribute::Strength)] / 2 + base_skill * 2
        + skills[index(Skill::Sect)] + (armed ? item_def(weapon).attack : 0);
}

int Hero::defense() const {
    return attributes[index(Attribute::Constitution)] / 4 + skills[index(Skill::Dodge)]
        + skills[index(Skill::Force)] + (armor != Item::Count ? item_def(armor).defense : 0);
}

int Hero::age() const { return 14 + static_cast<int>(steps / 2500); }

int Hero::amount(Item item) const { return bag[index(item)]; }

bool Hero::add(Item item, int count) {
    int total = amount(item) + count;
    if (total < 0 || total > inventory_limit) return false;
    bag[index(item)] = static_cast<uint16_t>(total);
    return true;
}

bool Hero::use(Item item) {
    if (!amount(item)) return false;
    switch (item) {
    case Item::Bun:
        if (food == food_limit) return false;
        food = std::min(food_limit, food + 45);
        break;
    case Item::Water:
        if (drink == drink_limit) return false;
        drink = std::min(drink_limit, drink + 45);
        break;
    case Item::Fish:
        if (food == food_limit && health == max_health()) return false;
        food = std::min(food_limit, food + 25);
        health = std::min(max_health(), health + 12);
        break;
    case Item::Medicine:
        if (health == max_health()) return false;
        health = std::min(max_health(), health + 35);
        break;
    default:
        const ItemDef& def = item_def(item);
        if (def.attack) { weapon = weapon == item ? Item::Count : item; return true; }
        if (def.defense) { armor = armor == item ? Item::Count : item; return true; }
        return false;
    }
    return add(item, -1);
}

bool Hero::train(Skill skill) {
    auto& level = skills[index(skill)];
    const int cost = 4 + level / 3;
    if (level == skill_limit || potential < cost || food == 0 || drink == 0 ||
        (skill == Skill::Sect && (sect == Sect::None || level >= skills[index(Skill::Fist)])))
        return false;
    potential -= cost;
    ++level;
    return true;
}

void Hero::advance() {
    if (steps < 200000) ++steps;
    if (steps % 12 == 0) {
        food = std::max(0, food - 1);
        drink = std::max(0, drink - 2);
    }
}

void Hero::restore() { health = max_health(); energy = max_energy(); }

void Hero::reward(int exp, int points, int money) {
    experience = std::min(stat_limit, experience + exp);
    potential = std::min(stat_limit, potential + points);
    coins = std::min(stat_limit, coins + money);
}

bool Hero::valid() const {
    if (appearance >= 3 || name >= 6 || index(sect) >= index(Sect::Count)) return false;
    if (std::accumulate(attributes.begin(), attributes.end(), 0) > 80) return false;
    for (int value : attributes) if (value < 10 || value > 30) return false;
    for (int count : bag) if (count > inventory_limit) return false;
    if (weapon != Item::Count &&
        (index(weapon) >= item_count || !item_def(weapon).attack || !amount(weapon))) return false;
    if (armor != Item::Count &&
        (index(armor) >= item_count || !item_def(armor).defense || !amount(armor))) return false;
    return health >= 1 && health <= max_health() && energy >= 0 && energy <= max_energy()
        && food >= 0 && food <= food_limit && drink >= 0 && drink <= drink_limit
        && experience >= 0 && experience <= stat_limit && potential >= 0 && potential <= stat_limit
        && coins >= 0 && coins <= stat_limit && steps <= 200000;
}

bool accepts(Sect sect, const Hero& hero) {
    switch (sect) {
    case Sect::Snow: return hero.attributes[index(Attribute::Agility)] >= 22;
    case Sect::Flower: return hero.appearance == 1;
    case Sect::Beast: return hero.attributes[index(Attribute::Insight)] <= 18;
    default: return sect != Sect::None;
    }
}

Hero new_hero(const Hero& draft) {
    Hero hero;
    hero.appearance = draft.appearance;
    hero.name = draft.name;
    hero.attributes = draft.attributes;
    hero.coins = 100;
    hero.potential = 100;
    hero.restore();
    return hero;
}

Exchange exchange(Hero& hero, const Opponent& opponent, int& enemy_health,
                  BattleAction action, uint32_t& state) {
    if (action == BattleAction::Flee) return {BattleResult::Escaped, 0, 0};
    if (action == BattleAction::Heal && !hero.use(Item::Medicine))
        return {BattleResult::Unavailable, 0, 0};
    const int special_cost = 8;
    if (action == BattleAction::Special) {
        if (hero.energy < special_cost) return {BattleResult::Unavailable, 0, 0};
        hero.energy -= special_cost;
    }
    int dealt = 0;
    if (action == BattleAction::Strike || action == BattleAction::Special) {
        const int aim = std::max(65, std::min(98, 85 +
            int(hero.attributes[index(Attribute::Agility)]) / 2 - opponent.agility));
        if (roll(state, 100) < aim || action == BattleAction::Special) {
            const int power = hero.attack() + (action == BattleAction::Special
                ? 7 + hero.skills[index(Skill::Sect)] * 2 : 0);
            dealt = std::min(enemy_health, std::max(1, power + roll(state, 5) - opponent.defense));
            enemy_health -= dealt;
        }
    }
    if (enemy_health <= 0) return {BattleResult::Victory, dealt, 0};
    const int blocked = hero.defense() + (action == BattleAction::Guard ? 12 : 0);
    const int received = std::min(hero.health, std::max(1, opponent.attack + roll(state, 5) - blocked));
    hero.health -= received;
    if (action == BattleAction::Guard) hero.energy = std::min(hero.max_energy(), hero.energy + 4);
    return {hero.health == 0 ? BattleResult::Defeat : BattleResult::Continue, dealt, received};
}
}
