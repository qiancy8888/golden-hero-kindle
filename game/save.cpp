#include "save.h"

#include <algorithm>
#include <climits>

namespace golden {
namespace {
constexpr uint8_t magic[] = {'G', 'H', 'K', 'P'};
constexpr uint32_t format = 1;
constexpr std::size_t size_v1 = 4 * (28 + attribute_count + skill_count + item_count);

void append(SaveBytes& bytes, uint32_t word) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<uint8_t>(word >> shift));
}

struct Reader {
    const SaveBytes& bytes;
    std::size_t pos;
    uint32_t word() {
        uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= uint32_t(bytes[pos++]) << shift;
        return value;
    }
    int number() {
        const uint32_t value = word();
        return value > INT_MAX ? -1 : static_cast<int>(value);
    }
};

uint32_t crc(const uint8_t* bytes, std::size_t size) {
    uint32_t result = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) {
        result ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit)
            result = (result >> 1) ^ ((result & 1) ? 0xedb88320u : 0u);
    }
    return ~result;
}
}

SaveBytes encode_save(const Journey& journey, uint32_t generation) {
    if (!generation || !journey.valid()) return {};
    const Hero& hero = journey.hero;
    SaveBytes bytes(magic, magic + sizeof(magic));
    append(bytes, format);
    append(bytes, generation);
    for (uint32_t value : {
        uint32_t(journey.area), uint32_t(journey.position.x), uint32_t(journey.position.y),
        uint32_t(journey.facing), uint32_t(journey.job), uint32_t(journey.hunt),
        uint32_t(journey.altar), uint32_t(journey.met_guide), uint32_t(journey.has_house),
        uint32_t(journey.merit), journey.random, uint32_t(hero.appearance), uint32_t(hero.name),
        uint32_t(hero.sect),
    }) append(bytes, value);
    for (int value : hero.attributes) append(bytes, value);
    for (int value : hero.skills) append(bytes, value);
    for (int value : hero.bag) append(bytes, value);
    for (uint32_t value : {
        uint32_t(hero.weapon), uint32_t(hero.armor), uint32_t(hero.health), uint32_t(hero.energy),
        uint32_t(hero.food), uint32_t(hero.drink), uint32_t(hero.experience), uint32_t(hero.potential),
        uint32_t(hero.coins), hero.steps,
    }) append(bytes, value);
    append(bytes, crc(bytes.data(), bytes.size()));
    return bytes;
}

SaveStatus decode_save(const SaveBytes& bytes, Journey& journey, uint32_t& generation) {
    if (bytes.size() < 16 || bytes.size() > save_limit ||
        !std::equal(magic, magic + sizeof(magic), bytes.begin())) return SaveStatus::Damaged;
    Reader read{bytes, sizeof(magic)};
    if (read.word() != format) return SaveStatus::Unsupported;
    if (bytes.size() != size_v1) return SaveStatus::Damaged;
    Reader trailer{bytes, bytes.size() - 4};
    if (trailer.word() != crc(bytes.data(), bytes.size() - 4)) return SaveStatus::Damaged;
    const uint32_t revision = read.word();
    if (!revision) return SaveStatus::Damaged;

    Journey decoded;
    decoded.area = static_cast<Area>(read.number());
    decoded.position = {read.number(), read.number()};
    decoded.facing = static_cast<Direction>(read.number());
    decoded.job = static_cast<Job>(read.number());
    decoded.hunt = static_cast<Hunt>(read.number());
    const auto altar = read.word();
    const auto guide = read.word();
    const auto house = read.word();
    const auto merit = read.word();
    decoded.random = read.word();
    if (altar > completed_altars || guide > 1 || house > 1 || merit > 9999) return SaveStatus::Damaged;
    decoded.altar = static_cast<uint8_t>(altar);
    decoded.met_guide = guide != 0;
    decoded.has_house = house != 0;
    decoded.merit = static_cast<uint16_t>(merit);
    auto& hero = decoded.hero;
    const auto appearance = read.word(), name = read.word();
    if (appearance >= 3 || name >= 6) return SaveStatus::Damaged;
    hero.appearance = static_cast<uint8_t>(appearance);
    hero.name = static_cast<uint8_t>(name);
    hero.sect = static_cast<Sect>(read.number());
    for (auto& value : hero.attributes) {
        const auto field = read.word();
        if (field > 30) return SaveStatus::Damaged;
        value = static_cast<uint8_t>(field);
    }
    for (auto& value : hero.skills) {
        const auto field = read.word();
        if (field > skill_limit) return SaveStatus::Damaged;
        value = static_cast<uint8_t>(field);
    }
    for (auto& value : hero.bag) {
        const auto field = read.word();
        if (field > inventory_limit) return SaveStatus::Damaged;
        value = static_cast<uint16_t>(field);
    }
    hero.weapon = static_cast<Item>(read.number());
    hero.armor = static_cast<Item>(read.number());
    hero.health = read.number();
    hero.energy = read.number();
    hero.food = read.number();
    hero.drink = read.number();
    hero.experience = read.number();
    hero.potential = read.number();
    hero.coins = read.number();
    hero.steps = read.word();

    if (read.pos != bytes.size() - 4 || !decoded.valid()) return SaveStatus::Damaged;
    journey = decoded;
    generation = revision;
    return SaveStatus::Loaded;
}
}
