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
    {u8"石料", 0, 0, 0}, {u8"钓杆", 24, 0, 0}, {u8"鲤鱼", 5, 0, 0},
    {u8"铁矿", 0, 0, 0}, {u8"自铸剑", 0, 28, 0},
    {u8"钢刀", 56, 14, 0}, {u8"钢杖", 58, 14, 0}, {u8"长鞭", 50, 12, 0},
};
constexpr SectDef sects[] = {
    {u8"无门无派", "", Skill::Legacy}, {u8"太极门", u8"清虚道长", Skill::TaijiFist},
    {u8"八卦门", u8"商剑鸣", Skill::BaguaArray}, {u8"雪山派", u8"白瑞德", Skill::SnowSword},
    {u8"花间派", u8"李青照", Skill::PlumHands}, {u8"红莲教", u8"红莲教教主", Skill::FoundingFist},
    {u8"伊贺谷", u8"伊贺上忍", Skill::FormlessFist}, {u8"兽王派", u8"娜可露", Skill::TigerFist},
    {u8"茅山派", u8"茅山道长", Skill::HeavenlyWay},
};
constexpr const char* names[] = {u8"无名", u8"云舟", u8"青岚", u8"飞雪", u8"长风", u8"明月"};
constexpr const char* appearances[] = {u8"少侠", u8"少女", u8"少年"};
constexpr const char* attributes[] = {u8"膂力", u8"敏捷", u8"悟性", u8"根骨"};
static_assert(sizeof(items) / sizeof(items[0]) == item_count, "One definition per item");
static_assert(sizeof(sects) / sizeof(sects[0]) == index(Sect::Count), "One definition per sect");

int roll(uint32_t& state, int maximum) {
    state = state * 1664525u + 1013904223u;
    return static_cast<int>((state >> 8) % static_cast<uint32_t>(maximum));
}
}

const char* hero_name(int name) { return names[name]; }
const char* appearance_name(int appearance) { return appearances[appearance]; }
const char* attribute_name(Attribute attribute) { return attributes[index(attribute)]; }
const ItemDef& item_def(Item item) { return items[index(item)]; }
const SectDef& sect_def(Sect sect) { return sects[index(sect)]; }

int Hero::max_health() const {
    return 60 + 2 * attributes[index(Attribute::Constitution)] + 3 * skills[index(Skill::Force)]
        + skills[index(best_style(Skill::Force))];
}

int Hero::max_energy() const {
    return 4 * skills[index(Skill::Force)] + 2 * skills[index(best_style(Skill::Force))];
}

Skill Hero::attack_foundation() const {
    switch (weapon) {
    case Item::SteelSaber: return Skill::Saber;
    case Item::SteelStaff: return Skill::Staff;
    case Item::LongWhip: return Skill::Whip;
    case Item::Count: return Skill::Fist;
    default: return Skill::Sword;
    }
}

Skill Hero::best_style(Skill foundation) const {
    Skill best = Skill::Legacy;
    for (std::size_t i = index(Skill::Beauty); i < skill_count; ++i) {
        const auto skill = static_cast<Skill>(i);
        const auto& def = skill_def(skill);
        if (def.foundation == foundation && def.sect == sect && skills[i] > skills[index(best)])
            best = skill;
    }
    return best;
}

Skill Hero::special_style() const {
    const Skill armed = best_style(attack_foundation());
    const Skill spell = best_style(Skill::Spell);
    return skills[index(spell)] > skills[index(armed)] ? spell : armed;
}

int Hero::attack() const {
    const Skill base = attack_foundation();
    return 5 + attributes[index(Attribute::Strength)] / 2 + 2 * skills[index(base)]
        + skills[index(best_style(base))] + (weapon != Item::Count ? item_def(weapon).attack : 0);
}

int Hero::defense() const {
    return attributes[index(Attribute::Constitution)] / 4 + skills[index(Skill::Dodge)]
        + skills[index(Skill::Force)] + skills[index(Skill::Parry)] / 2
        + skills[index(best_style(Skill::Dodge))] / 2 + skills[index(best_style(Skill::Force))] / 2
        + skills[index(best_style(Skill::Parry))] / 2
        + (armor != Item::Count ? item_def(armor).defense : 0);
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

bool Hero::can_study(Skill skill) const {
    if (index(skill) >= skill_count || skill == Skill::Legacy) return false;
    const auto& def = skill_def(skill);
    return def.sect == Sect::None || def.sect == sect;
}

bool Hero::train(Skill skill) {
    if (!can_study(skill)) return false;
    auto& level = skills[index(skill)];
    const auto& def = skill_def(skill);
    const int cost = 4 + level / 3;
    if (level == skill_limit || potential < cost || food == 0 || drink == 0 ||
        (def.foundation != skill && level >= skills[index(def.foundation)]))
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
    if (skills[index(Skill::Legacy)]) return false;
    for (std::size_t i = index(Skill::Beauty); i < skill_count; ++i)
        if (skills[i] && !can_study(static_cast<Skill>(i))) return false;
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
                ? 7 + hero.skills[index(hero.special_style())] * 2 : 0);
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
