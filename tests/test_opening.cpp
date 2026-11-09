#include "town.h"
#include "view.h"

#include <cstdio>
#include <cstdlib>

using namespace golden;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition); \
        std::exit(1); \
    } \
} while (false)

namespace {
void walk(Town& town, Direction direction, int steps) {
    for (int i = 0; i < steps; ++i) CHECK(town.move(direction));
}
}

int main() {
    Town town;
    CHECK(town.page() == Page::Title);
    CHECK(!town.move(Direction::Left));
    CHECK(!town.back());
    CHECK(town.confirm() && town.area() == Area::Center);
    CHECK(town.nearby() && !town.move(Direction::Up));
    CHECK(town.player().x == 9 && town.player().y == 5);

    Frame before, after;
    render(town, before);
    CHECK(changed_region(before, before).w == 0);
    CHECK(town.confirm() && town.page() == Page::Dialogue);
    CHECK(!town.move(Direction::Right));
    CHECK(town.back() && town.page() == Page::Town);

    walk(town, Direction::Right, 1);
    render(town, after);
    const Rect moved = changed_region(before, after);
    CHECK(moved.w > 0 && moved.w < view_width && moved.x % 8 == 0);
    CHECK(!town.confirm());
    walk(town, Direction::Right, 9);
    CHECK(town.area() == Area::East && town.player().x == 0);
    walk(town, Direction::Right, 4);
    CHECK(town.confirm() && town.confirm());
    CHECK(town.job() == Job::Available);
    CHECK(town.experience() == starting_experience && town.potential() == 0);

    walk(town, Direction::Left, 24);
    CHECK(town.area() == Area::West && town.player().x == 18);
    town.move(Direction::Up);
    CHECK(town.player().y == 5);
    walk(town, Direction::Left, 4);
    CHECK(town.nearby() && town.confirm());
    CHECK(town.back() && town.job() == Job::Available);
    CHECK(town.confirm() && town.confirm() && town.job() == Job::Carrying);
    CHECK(town.confirm() && town.confirm() && town.job() == Job::Carrying);

    walk(town, Direction::Right, 28);
    CHECK(town.area() == Area::East && town.nearby());
    CHECK(town.confirm() && town.back() && town.job() == Job::Carrying);
    CHECK(town.confirm() && town.confirm() && town.job() == Job::Complete);
    CHECK(town.page() == Page::Dialogue);
    CHECK(town.experience() == starting_experience + 40 && town.potential() == 20);
    CHECK(town.confirm() && town.confirm() && town.confirm());
    CHECK(town.experience() == starting_experience + 40 && town.potential() == 20);

    CHECK(hit_test({90, 386}) == Action::Up);
    CHECK(hit_test({16, 435}) == Action::None);
    CHECK(hit_test({17, 435}) == Action::Left);
    CHECK(hit_test({240, 390}) == Action::Confirm);
    CHECK(hit_test({300, 447}) == Action::Exit);
    CHECK(hit_test({300, 461}) == Action::None);
    std::puts("Standalone opening checks passed.");
}
