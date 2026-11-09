#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace golden {
enum class Attribute { Strength, Agility, Insight, Constitution, Count };
enum class Item { Bun, Water, Medicine, WoodSword, IronSword, Coat, Stone,
                  FishingRod, Fish, Iron, ForgedSword, Count };
enum class Skill { Fist, Sword, Dodge, Force, Sect, Count };
enum class Sect { None, Taiji, Bagua, Snow, Flower, Lotus, Iga, Beast, Maoshan, Count };

constexpr int skill_limit = 255;
constexpr int inventory_limit = 999;
constexpr int stat_limit = 100000000;
constexpr std::size_t attribute_count = static_cast<std::size_t>(Attribute::Count);
constexpr std::size_t item_count = static_cast<std::size_t>(Item::Count);
constexpr std::size_t skill_count = static_cast<std::size_t>(Skill::Count);
constexpr int altar_count = 8;

struct ItemDef {
    const char* name;
    int price;
    int attack;
    int defense;
};

struct SectDef {
    const char* name;
    const char* art;
    const char* master;
};

struct Hero {
    uint8_t appearance = 0;
    uint8_t name = 0;
    std::array<uint8_t, attribute_count> attributes{{20, 20, 20, 20}};
    Sect sect = Sect::None;
    std::array<uint8_t, skill_count> skills{};
    std::array<uint16_t, item_count> bag{};
    Item weapon = Item::Count;
    Item armor = Item::Count;
    int health = 0;
    int energy = 0;
    int food = 100;
    int drink = 100;
    int experience = 0;
    int potential = 0;
    int coins = 0;
    uint32_t steps = 0;

    int max_health() const;
    int max_energy() const;
    int attack() const;
    int defense() const;
    int age() const;
    int amount(Item item) const;
    bool add(Item item, int count);
    bool use(Item item);
    bool train(Skill skill);
    void advance();
    void restore();
    void reward(int exp, int points, int money);
    bool valid() const;
};

const char* hero_name(int name);
const char* appearance_name(int appearance);
const char* attribute_name(Attribute attribute);
const char* skill_name(Skill skill);
const ItemDef& item_def(Item item);
const SectDef& sect_def(Sect sect);
bool accepts(Sect sect, const Hero& hero);
Hero new_hero(const Hero& draft);

struct Opponent {
    const char* name;
    int health;
    int attack;
    int defense;
    int agility;
    int experience;
    int potential;
    int coins;
};

enum class BattleAction { Strike, Special, Guard, Heal, Flee };
enum class BattleResult { Continue, Victory, Defeat, Escaped, Unavailable };

struct Exchange {
    BattleResult result;
    int dealt;
    int received;
};

Exchange exchange(Hero& hero, const Opponent& opponent, int& enemy_health,
                  BattleAction action, uint32_t& random_state);
}
