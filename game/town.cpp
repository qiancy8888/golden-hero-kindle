#include "town.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <numeric>

namespace golden {
namespace {
constexpr int stone_minimum = 1000;
constexpr int altar_minimum = 1000;
constexpr int create_rows = 7;

template <typename E> int id(E value) { return static_cast<int>(value); }

std::string numbered(const char* name, int value) {
    return std::string(name) + "  " + std::to_string(value);
}

bool stocks(Role vendor, Item item) {
    const auto& def = item_def(item);
    if (!def.price) return false;
    switch (vendor) {
    case Role::Armorer: return def.attack || def.defense;
    case Role::Healer: return item == Item::Medicine;
    case Role::Innkeeper: return item == Item::Bun || item == Item::Water;
    default: return item == Item::Bun || item == Item::Water || item == Item::FishingRod;
    }
}
}

bool Journey::valid() const {
    return id(area) >= 0 && area < Area::Count && id(facing) >= 0 && facing <= Direction::Right
        && id(job) >= 0 && job <= Job::Complete && id(hunt) >= 0 && hunt <= Hunt::Report
        && altar <= completed_altars && merit <= 9999 && may_enter(area, altar)
        && hero.valid() && walkable(area, position)
        && hero.amount(Item::Stone) == (job == Job::Carrying ? 1 : 0);
}

const Resident* Town::nearby() const {
    const auto* front = at(area(), offset(player(), facing()));
    if (front) return front;
    for (auto direction : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        const auto* person = at(area(), offset(player(), direction));
        if (person) return person;
    }
    return nullptr;
}

Town::Town() { refresh(); }

const char* Town::journal() const {
    if (state_.altar == completed_altars) return u8"暗黑势力已平";
    if (state_.hunt == Hunt::Report) return u8"回衙门向捕快复命";
    if (state_.hunt == Hunt::Tracking) return u8"前往镇东捉拿恶人";
    if (state_.job == Job::Carrying) return u8"把石料送到镇东工地";
    if (state_.altar) return location_name(altar_area(state_.altar));
    if (hero().experience >= altar_minimum) return u8"向村长打听暗黑据点";
    return u8"逛平安镇，向书童打听消息";
}

const char* Town::title() const {
    switch (page_) {
    case Page::Title: return u8"黄金英雄坛说";
    case Page::Create: return u8"创建人物";
    case Page::Town: return location();
    case Page::Actions: return target_->name;
    case Page::Menu: return u8"江湖菜单";
    case Page::Status: return u8"人物状态";
    case Page::Inventory: return u8"背包与装备";
    case Page::Skills: return u8"武学修为";
    case Page::Shop: return target_->name;
    case Page::Battle: return enemy_->name;
    case Page::Ending: return u8"暗黑坛之战";
    }
    return "";
}

const char* Town::confirm_label() const {
    if (page_ == Page::Town) return nearby() ? u8"交谈" : u8"查看";
    if (page_ == Page::Create && selected_ < create_rows - 1) return u8"下一项";
    return u8"确定";
}

std::string Town::status_line() const {
    char line[100];
    if (page_ == Page::Battle)
        std::snprintf(line, sizeof(line), u8"气血 %d  内力 %d  对手 %d", hero().health, hero().energy, enemy_health_);
    else
        std::snprintf(line, sizeof(line), u8"气血 %d/%d   经验 %d   钱 %d",
                      hero().health, hero().max_health(), hero().experience, hero().coins);
    return line;
}

std::string Town::gate_hint() const {
    for (auto direction : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        const auto edge = gate(direction);
        if (std::abs(edge.x - player().x) + std::abs(edge.y - player().y) > 2) continue;
        const auto* link = passage(area(), direction);
        if (!link) continue;
        const Area next = link->from == area() ? link->to : link->from;
        return std::string(may_enter(next, state_.altar) ? u8"此处通往 " : u8"地图未得：")
            + location_name(next);
    }
    Area objective = Area::Center;
    if (state_.hunt == Hunt::Report) objective = Area::Court;
    else if (state_.hunt == Hunt::Tracking || state_.job == Job::Carrying) objective = Area::East;
    else if (state_.altar > 0 && state_.altar <= altar_count) objective = altar_area(state_.altar);
    Direction direction;
    if (route_step(area(), objective, state_.altar, direction)) {
        constexpr const char* sides[] = {u8"北", u8"南", u8"西", u8"东"};
        return std::string(u8"任务方向：") + sides[id(direction)] + u8"出口";
    }
    return {};
}

void Town::add(std::string label, Op op, int value) {
    menu_.push_back({std::move(label), op, value});
}

void Town::open(Page page) {
    page_ = page;
    selected_ = 0;
    notice_.clear();
    refresh();
}

void Town::refresh() {
    menu_.clear();
    switch (page_) {
    case Page::Title:
        if (has_hero_) add(u8"继续江湖", Op::Resume);
        add(u8"新建人物", Op::Create);
        break;
    case Page::Create:
        add(std::string(u8"人物  ") + appearance_name(draft_.appearance), Op::Begin);
        add(std::string(u8"姓名  ") + hero_name(draft_.name), Op::Begin);
        for (int i = 0; i < id(Attribute::Count); ++i)
            add(numbered(attribute_name(static_cast<Attribute>(i)), draft_.attributes[i]), Op::Begin);
        add(u8"进入平安镇", Op::Begin);
        break;
    case Page::Menu:
        add(u8"人物状态", Op::Status);
        add(u8"背包与装备", Op::Inventory);
        add(u8"武学修为", Op::Skills);
        add(u8"保存进度", Op::Save);
        if (state_.altar == completed_altars) add(u8"查看结局", Op::ReadEnding);
        add(u8"返回地图", Op::Return);
        break;
    case Page::Status:
        add(std::string(hero_name(hero().name)) + "  " + appearance_name(hero().appearance), Op::Return);
        add(std::string(u8"门派  ") + sect_def(hero().sect).name, Op::Return);
        add(numbered(u8"年龄", hero().age()) + "   " + numbered(u8"潜能", hero().potential), Op::Return);
        add(numbered(u8"食物", hero().food) + "   " + numbered(u8"饮水", hero().drink), Op::Return);
        add(numbered(u8"内力", hero().energy) + "   " + numbered(u8"善名", state_.merit), Op::Return);
        add(numbered(u8"攻击", hero().attack()) + "   " + numbered(u8"防御", hero().defense()), Op::Return);
        break;
    case Page::Inventory:
        for (int i = 0; i < id(Item::Count); ++i) {
            const Item item = static_cast<Item>(i);
            if (!hero().amount(item)) continue;
            auto label = numbered(item_def(item).name, hero().amount(item));
            if (hero().weapon == item || hero().armor == item) label += u8"  已装备";
            add(label, Op::Use, i);
        }
        if (menu_.empty()) add(u8"背包是空的", Op::Return);
        break;
    case Page::Skills:
        for (int i = 0; i < id(Skill::Count); ++i) {
            const Skill skill = static_cast<Skill>(i);
            if (hero().can_study(skill))
                add(numbered(skill_name(skill), hero().skills[i]), Op::Train, i);
        }
        break;
    case Page::Actions:
        interact();
        break;
    case Page::Shop:
        for (int i = 0; i < id(Item::Count); ++i) {
            const Item item = static_cast<Item>(i);
            if (stocks(target_->role, item))
                add(numbered(item_def(item).name, item_def(item).price) + u8" 文", Op::Buy, i);
        }
        if (target_->role == Role::Vendor && hero().amount(Item::Fish)) add(u8"卖一尾鲤鱼", Op::SellFish);
        break;
    case Page::Battle: {
        const Skill style = hero().special_style();
        add(u8"普通攻击", Op::Fight, id(BattleAction::Strike));
        add(style == Skill::Legacy ? u8"运力一击" : skill_name(style),
            Op::Fight, id(BattleAction::Special));
        add(u8"防守调息", Op::Fight, id(BattleAction::Guard));
        add(u8"服用金创药", Op::Fight, id(BattleAction::Heal));
        add(u8"撤退", Op::Fight, id(BattleAction::Flee));
        break;
    }
    case Page::Ending:
        add(u8"继续游历", Op::Return);
        break;
    case Page::Town:
        break;
    }
    selected_ = std::min(selected_, std::max(0, menu_size() - 1));
}

void Town::interact() {
    switch (target_->role) {
    case Role::Elder:
        if (state_.altar == 0 && hero().experience >= altar_minimum) add(u8"领取青龙坛地图", Op::LearnMap);
        if (state_.altar == completed_altars) add(u8"向村长报捷", Op::ReadEnding);
        if (menu_.empty()) notice_ = u8"先到武馆请教，去衙门接差事。";
        break;
    case Role::Guide: add(u8"打听镇上的消息", Op::Guide); break;
    case Role::StoneKeeper:
        if (state_.job != Job::Carrying && hero().experience >= stone_minimum)
            add(u8"领石料", Op::TakeStone);
        else notice_ = state_.job == Job::Carrying ? u8"将石料交给镇东工地管事。" : u8"经验满一千再来领活。";
        break;
    case Role::Foreman:
        if (state_.job == Job::Carrying) add(u8"交石料", Op::DeliverStone);
        else notice_ = u8"有石料才可结算，先去镇西。";
        break;
    case Role::Vendor: case Role::Armorer: add(u8"买卖物品", Op::Shop); break;
    case Role::Innkeeper: add(u8"买饭和水", Op::Shop); add(u8"住店（六文）", Op::Rest, 6); break;
    case Role::Guard:
        if (state_.hunt == Hunt::None) add(u8"领取除恶差事", Op::Bounty);
        if (state_.hunt == Hunt::Report) add(u8"领取差事赏金", Op::ReportBounty);
        if (state_.hunt == Hunt::Tracking) notice_ = u8"恶人在平安镇东，仔细搜寻。";
        break;
    case Role::Healer: add(u8"疗伤（八文）", Op::Rest, 8); add(u8"买药", Op::Shop); break;
    case Role::Trainer: add(u8"请教基本功", Op::Skills); break;
    case Role::Veteran:
        if (hero().experience >= 200000) add(u8"请教基本功", Op::Skills);
        else notice_ = u8"经验还浅，先找武师教头。";
        break;
    case Role::Master:
        if (hero().sect == Sect::None) add(u8"拜师入门", Op::Join);
        else if (hero().sect == local_sect(area())) add(u8"向师父请教", Op::Skills);
        else notice_ = u8"你已经拜入别的门派。";
        break;
    case Role::Bandit: add(u8"出手缉拿", Op::Battle); break;
    case Role::AltarLord:
        if (altar_number(area()) == state_.altar) add(u8"挑战坛主", Op::Battle);
        else notice_ = u8"此坛已经攻破。";
        break;
    case Role::Smith:
        add(u8"采集铁矿", Op::Mine);
        add(u8"铸剑（三铁矿、百文）", Op::Forge);
        break;
    case Role::Fisher: add(u8"持钓杆垂钓", Op::Fish); break;
    case Role::Keeper:
        if (area() == Area::Peach && !state_.has_house) add(u8"置办家园（二百文）", Op::BuyHouse);
        else add(u8"歇息调养", Op::Rest);
        break;
    case Role::Judge: add(u8"返回阳间", Op::Revive); break;
    case Role::None: break;
    }
    if (menu_.empty()) add(u8"结束交谈", Op::Return);
}

void Town::change_draft(int amount) {
    if (selected_ == 0) draft_.appearance = (draft_.appearance + amount + 3) % 3;
    else if (selected_ == 1) draft_.name = (draft_.name + amount + 6) % 6;
    else if (selected_ < create_rows - 1) {
        auto& value = draft_.attributes[selected_ - 2];
        const int total = std::accumulate(draft_.attributes.begin(), draft_.attributes.end(), 0);
        if (value + amount >= 10 && value + amount <= 30 && total + amount <= 80)
            value = static_cast<uint8_t>(value + amount);
    }
    refresh();
}

bool Town::move(Direction direction) {
    if (page_ != Page::Town) {
        if (page_ == Page::Create && (direction == Direction::Left || direction == Direction::Right)) {
            change_draft(direction == Direction::Right ? 1 : -1);
            return true;
        }
        if (menu_.empty() || (direction != Direction::Up && direction != Direction::Down)) return false;
        selected_ = (selected_ + menu_size() + (direction == Direction::Down ? 1 : -1)) % menu_size();
        return true;
    }

    const Point next = offset(player(), direction);
    const bool turned = direction != facing();
    state_.facing = direction;
    if (player() == gate(direction)) {
        const auto* link = passage(area(), direction);
        if (link) {
            const bool forward = area() == link->from;
            const Area destination = forward ? link->to : link->from;
            if (!may_enter(destination, state_.altar)) {
                notice_ = u8"需要相应的据点地图。";
                return true;
            }
            state_.area = destination;
            state_.position = gate(forward ? link->arrive : link->leave);
            state_.hero.advance();
            notice_.clear();
            return true;
        }
    }
    if (!walkable(area(), next)) return turned;
    state_.position = next;
    state_.hero.advance();
    notice_.clear();
    return true;
}

bool Town::confirm() {
    if (page_ == Page::Town) {
        target_ = nearby();
        open(target_ ? Page::Actions : Page::Menu);
        return true;
    }
    if (page_ == Page::Create && selected_ < create_rows - 1) {
        ++selected_;
        return true;
    }
    if (menu_.empty()) return false;
    const Entry entry = menu_[selected_];
    execute(entry);
    return true;
}

bool Town::back() {
    switch (page_) {
    case Page::Title: return false;
    case Page::Create: open(Page::Title); break;
    case Page::Town: open(Page::Menu); break;
    case Page::Battle: combat(BattleAction::Flee); break;
    case Page::Shop: open(Page::Actions); break;
    case Page::Inventory: case Page::Skills: case Page::Status: open(Page::Menu); break;
    default: open(Page::Town); break;
    }
    return true;
}

bool Town::menu() {
    if (page_ == Page::Town) return back();
    return false;
}

bool Town::resume(const Journey& saved) {
    if (!saved.valid()) return false;
    state_ = saved;
    has_hero_ = true;
    open(Page::Title);
    return true;
}

bool Town::can_save() const { return has_hero_ && page_ != Page::Create && page_ != Page::Battle; }

bool Town::take_save_request() {
    const bool requested = save_pending_;
    save_pending_ = false;
    return requested;
}

void Town::save_result(bool success) {
    if (!success) notice_ = u8"保存失败，旧档仍可读取。";
    else if (save_feedback_) notice_ = u8"进度已保存。";
    save_feedback_ = false;
}

void Town::record(const char* message) {
    notice_ = message;
    save_pending_ = true;
}

void Town::execute(const Entry& entry) {
    Hero& hero = state_.hero;
    switch (entry.op) {
    case Op::Create: draft_ = Hero{}; open(Page::Create); break;
    case Op::Resume: open(Page::Town); break;
    case Op::Begin:
        state_ = Journey{};
        state_.hero = new_hero(draft_);
        has_hero_ = true;
        open(Page::Town);
        record(u8"十四岁初入江湖，可向村民打听。");
        break;
    case Op::Status: open(Page::Status); break;
    case Op::Inventory: open(Page::Inventory); break;
    case Op::Skills: open(Page::Skills); break;
    case Op::Shop: open(Page::Shop); break;
    case Op::Return: open(Page::Town); break;
    case Op::Save: save_feedback_ = save_pending_ = true; break;
    case Op::Guide:
        if (!state_.met_guide) {
            state_.met_guide = true;
            hero.reward(80, 20, 12);
            hero.add(Item::WoodSword, 1);
            record(u8"小书童送你一把木剑。");
        } else notice_ = u8"衙门招人除恶，武馆可请教。";
        break;
    case Op::LearnMap:
        state_.altar = starting_altar;
        open(Page::Town);
        record(u8"取得青龙坛地图，入口在商家堡。");
        break;
    case Op::TakeStone:
        state_.job = Job::Carrying;
        hero.add(Item::Stone, 1);
        open(Page::Town);
        record(u8"已背起石料，往镇东工地走。");
        break;
    case Op::DeliverStone: {
        const int exp = 40 + hero.experience / 1500;
        state_.job = Job::Complete;
        hero.add(Item::Stone, -1);
        hero.reward(exp, exp / 2, 8);
        open(Page::Town);
        notice_ = numbered(u8"交付石料。经验 +", exp) + numbered(u8"  潜能 +", exp / 2);
        save_pending_ = true;
        break;
    }
    case Op::Bounty:
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
        if (!hero.amount(Item::FishingRod)) { notice_ = u8"先买一支钓杆。"; break; }
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
