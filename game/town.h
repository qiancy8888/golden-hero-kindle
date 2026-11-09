#pragma once

#include "world.h"

#include <string>
#include <vector>

namespace golden {
enum class Page { Title, Create, Town, Actions, Menu, Status, Inventory, Skills, Shop, Battle, Ending };
enum class Job { Available, Carrying, Complete };
enum class Hunt { None, Tracking, Report };

struct Journey {
    Hero hero;
    Area area = Area::Center;
    Point position{9, 5};
    Direction facing = Direction::Up;
    Job job = Job::Available;
    Hunt hunt = Hunt::None;
    uint8_t altar = 0;
    bool met_guide = false;
    bool has_house = false;
    uint16_t merit = 0;
    uint32_t random = 0x47a91dc3;
    bool valid() const;
};

class Town {
public:
    Town();
    Page page() const { return page_; }
    Area area() const { return state_.area; }
    Point player() const { return state_.position; }
    Direction facing() const { return state_.facing; }
    const Journey& journey() const { return state_; }
    const Hero& hero() const { return state_.hero; }
    const Resident* nearby() const;
    const char* location() const { return location_name(area()); }
    const char* journal() const;
    const std::string& notice() const { return notice_; }
    const char* title() const;
    const char* confirm_label() const;
    std::string status_line() const;
    std::string gate_hint() const;
    int selection() const { return selected_; }
    int menu_size() const { return static_cast<int>(menu_.size()); }
    const std::string& menu_label(int row) const { return menu_[row].label; }

    bool move(Direction direction);
    bool confirm();
    bool back();
    bool menu();
    bool resume(const Journey& saved);
    bool can_save() const;
    bool take_save_request();
    void save_result(bool success);

private:
    enum class Op { Create, Resume, Begin, Status, Inventory, Skills, Save, Return, Guide, LearnMap,
                    TakeStone, DeliverStone, Bounty, ReportBounty, Shop, Buy, SellFish, Train,
                    Join, Battle, Fight, Use, Rest, Fish, Mine, Forge, BuyHouse, Revive, ReadEnding };
    struct Entry { std::string label; Op op; int value; };

    void open(Page page);
    void refresh();
    void add(std::string label, Op op, int value = 0);
    void execute(const Entry& entry);
    void interact();
    void change_draft(int amount);
    void combat(BattleAction action);
    void record(const char* message);

    Journey state_;
    Hero draft_;
    Page page_ = Page::Title;
    const Resident* target_ = nullptr;
    const Opponent* enemy_ = nullptr;
    int enemy_health_ = 0;
    int selected_ = 0;
    bool has_hero_ = false;
    bool save_pending_ = false;
    bool save_feedback_ = false;
    std::string notice_;
    std::vector<Entry> menu_;
};
}
