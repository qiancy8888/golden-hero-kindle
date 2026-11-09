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
    return 30 + 2 * attributes[index(Attribute::Constitution)] + 3 * skills[index(Skill::Force)];
}

int Hero::max_energy() const {
    return 10 + attributes[index(Attribute::Constitution)] + 4 * skills[index(Skill::Force)];
}

int Hero::attack() const {
    const bool armed = weapon != Item::Count;
    const int base_skill = skills[index(armed ? Skill::Sword : Skill::Fist)];
    return 5 + attributes[in…7984 tokens truncated…p::Bounty:
        state_.hunt = Hunt::Tracking;
        open(Page::Town);
        record(u8"到平安镇东缉拿恶人。");
        break;
    case Op::ReportBounty:
        state_.hunt = Hunt::None;
        hero.reward(250 + hero.experience / 10, 65, 36);
        if (state_.merit < 9999) ++state_.merit;
        open(Page::Town);
        record(u8"捕快已发放经验、潜能和赏金。");
        break;
    case Op::Buy: {
        const Item item = static_cast<Item>(entry.value);
        const int price = item_def(item).price;
        if (hero.coins < price) notice_ = u8"钱不够。";
        else if (!hero.add(item, 1)) notice_ = u8"背包放不下了。";
        else { hero.coins -= price; record(u8"已放入背包。"); refresh(); }
        break;
    }
    case Op::SellFish:
        if (hero.add(Item::Fish, -1)) { hero.reward(0, 0, 5); refresh(); record(u8"卖出一尾鲤鱼。"); }
        break;
    case Op::Train:
        if (hero.train(static_cast<Skill>(entry.value))) { refresh(); record(u8"武学提升一级。"); }
        else notice_ = u8"请检查潜能、饮食或基本功。";
        break;
    case Op::Join: {
        const Sect sect = local_sect(area());
        if (!accepts(sect, hero)) { notice_ = u8"人物条件未达到入门要求。"; break; }
        hero.sect = sect;
        open(Page::Town);
        record(u8"拜师成功，可向师父请教。");
        break;
    }
    case Op::Battle:
        enemy_ = &opponent(target_->role, area());
        enemy_health_ = enemy_->health;
        open(Page::Battle);
        break;
    case Op::Fight: combat(static_cast<BattleAction>(entry.value)); break;
    case Op::Use:
        if (hero.use(static_cast<Item>(entry.value))) { refresh(); record(u8"已使用或调整装备。"); }
        else notice_ = u8"现在用不上这件物品。";
        break;
    case Op::Rest:
        if (hero.coins < entry.value) { notice_ = u8"钱不够。"; break; }
        hero.coins -= entry.value;
        hero.restore();
        if (!entry.value) { hero.food = 100; hero.drink = 100; }
        record(u8"气血和内力已恢复。");
        break;
    case Op::Fish:
        if (!hero.amount(Item::FishingRod)) { notice_ = u8"先买一支钓竿。"; break; }
        if (hero.add(Item::Fish, 1)) { hero.advance(); record(u8"钓到一尾鲤鱼。"); }
        else notice_ = u8"鱼篓已经装满。";
        break;
    case Op::Mine:
        if (hero.add(Item::Iron, 1)) { hero.advance(); record(u8"拾到一块铁矿。"); }
        else notice_ = u8"背包放不下了。";
        break;
    case Op::Forge:
        if (hero.amount(Item::Iron) < 3 || hero.coins < 100) { notice_ = u8"还缺铁矿或钱。"; break; }
        if (!hero.add(Item::ForgedSword, 1)) { notice_ = u8"背包放不下了。"; break; }
        hero.add(Item::Iron, -3);
        hero.coins -= 100;
        record(u8"铸成一柄新剑。");
        break;
    case Op::BuyHouse:
        if (hero.coins < 200) { notice_ = u8"还凑不齐二百文。"; break; }
        hero.coins -= 200;
        state_.has_house = true;
        refresh();
        record(u8"你在桃花源有了一处宅院。");
        break;
    case Op::Revive:
        hero.coins -= hero.coins / 10;
        hero.restore();
        hero.food = 100;
        hero.drink = 100;
        state_.area = Area::Home;
        state_.position = {9, 5};
        open(Page::Town);
        record(u8"你醒在家里，少了些盘缠。");
        break;
    case Op::ReadEnding:
        open(Page::Ending);
        notice_ = u8"八坛平定，平安镇重归安宁。";
        break;
    }
}

void Town::combat(BattleAction action) {
    const Exchange result = exchange(state_.hero, *enemy_, enemy_health_, action, state_.random);
    switch (result.result) {
    case BattleResult::Unavailable: notice_ = u8"内力不足或此时无法用药。"; break;
    case BattleResult::Continue:
        notice_ = numbered(u8"你打出 ", result.dealt) + numbered(u8" 点，受伤 ", result.received);
        break;
    case BattleResult::Escaped:
        open(Page::Town);
        record(u8"你离开了战斗。");
        break;
    case BattleResult::Defeat:
        state_.hero.health = 1;
        state_.hero.energy = 0;
        state_.area = Area::Underworld;
        state_.position = {9, 5};
        open(Page::Town);
        record(u8"你败下阵来，去见阎罗王。");
        break;
    case BattleResult::Victory:
        state_.hero.reward(enemy_->experience, enemy_->potential, enemy_->coins);
        if (target_->role == Role::Bandit && state_.hunt == Hunt::Tracking) state_.hunt = Hunt::Report;
        if (target_->role == Role::AltarLord) ++state_.altar;
        if (state_.altar == completed_altars) {
            open(Page::Ending);
            record(u8"八坛平定，平安镇重归安宁。");
        } else {
            open(Page::Town);
            record(target_->role == Role::AltarLord ? u8"攻破据点，获得下一坛地图。" : u8"获胜，得到经验、潜能和铜钱。");
        }
        break;
    }
}
}
