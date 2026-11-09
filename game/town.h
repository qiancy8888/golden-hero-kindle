#pragma once

namespace golden {
constexpr int town_width = 19;
constexpr int town_height = 11;
constexpr int starting_experience = 1000;

enum class Direction { Up, Down, Left, Right };
enum class Area { West, Center, East };
enum class Page { Title, Town, Dialogue };
enum class Job { Available, Carrying, Complete };
enum class Tile { Grass, Road, Wall, Tree, Stones, Water };

struct Point {
    int x;
    int y;
};

struct Resident {
    Point position;
    const char* name;
};

class Town {
public:
    Page page() const { return page_; }
    Area area() const { return area_; }
    Point player() const { return player_; }
    Direction facing() const { return facing_; }
    Job job() const { return job_; }
    int experience() const { return experience_; }
    int potential() const { return potential_; }
    const Resident& resident() const;
    Tile tile(Point point) const;
    bool nearby() const;
    const char* location() const;
    const char* journal() const;
    const char* dialogue() const;
    const char* confirm_label() const;

    bool move(Direction direction);
    bool confirm();
    bool back();

private:
    Page page_ = Page::Title;
    Area area_ = Area::Center;
    Point player_ = {9, 5};
    Direction facing_ = Direction::Up;
    Job job_ = Job::Available;
    int experience_ = starting_experience;
    int potential_ = 0;
};
}
