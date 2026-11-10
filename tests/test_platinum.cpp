#include "platinum_core.h"

#include <cstdio>
#include <cstring>
#include <libretro.h>

using namespace platinum;

namespace {
bool check(bool condition, const char* message) {
    if (!condition) std::fprintf(stderr, "%s\n", message);
    return condition;
}

void advance(Button button, int frames) {
    for (int i = 0; i < frames; ++i) run(button);
}

void confirm() {
    advance(Button::A, 6);
    advance(Button::None, 60);
}

bool test_monk() {
    const auto* ram = static_cast<const uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM));
    if (!check(ram != nullptr, "Gambatte must expose the game's work RAM")) return false;
    // WRAM symbols from the pinned GMUD_GB build: G_Curr_Map, located_id.
    constexpr size_t map = 0xCE5;
    constexpr size_t npc = 0xD22;
    const auto world_x = [ram] { return 32 * ram[0xD13] + ram[0xD15] + ram[0xD1C]; };
    if (!check(ram[map] == 1, "The restored character must start at home")) return false;
    const Button route[] = {Button::Down, Button::Right, Button::Right, Button::Right,
        Button::Right, Button::Down, Button::Down, Button::Down, Button::Right};
    for (Button direction : route) {
        advance(direction, 2);
        advance(Button::None, 45);
    }
    if (!check(ram[map] == 2 && ram[npc] == 12, "The route must reach Daode the monk")) return false;
    const Frame outside = frame();
    confirm();
    const Frame menu = frame();
    if (!check(menu != outside, "The monk's menu must open")) return false;
    confirm();
    const Frame dialogue = frame();
    if (!check(dialogue != menu && dialogue != outside, "Selecting Talk must show dialogue")) return false;
    for (int page = 0; page < 8 && frame() != outside; ++page) confirm();
    const int x = world_x();
    advance(Button::Left, 2);
    advance(Button::None, 45);
    return check(world_x() < x && ram[map] == 2,
                 "The character must be able to walk after the conversation");
}

bool test_bounds() {
    Frame before{}, after{};
    if (!check(changed_region(before, after).w == 0, "Identical frames are clean")) return false;
    after[8 * width + 12] = 255;
    after[20 * width + 7] = 1;
    const Region dirty = changed_region(before, after);
    return check(dirty.x == 7 && dirty.y == 8 && dirty.w == 6 && dirty.h == 13,
                 "Dirty bounds enclose exactly the changed pixels");
}

bool test_refresh_schedule() {
    RefreshSchedule short_walk(0);
    bool ok = check(!short_walk.ready(299) && short_walk.ready(300),
                    "Submissions must respect the 300 ms panel interval");
    ok &= check(short_walk.ready(1000) && short_walk.choose(true) == RefreshAction::Partial,
                "A changed frame from a short tap must be eligible immediately after idle");
    short_walk.defer(1000);
    ok &= check(!short_walk.ready(1099) && short_walk.ready(1100) &&
                short_walk.choose(true) == RefreshAction::Partial,
                "A busy EPDC must defer the newest frame without consuming it");

    RefreshSchedule continuous(0);
    unsigned partials = 0;
    for (uint64_t now = 1000; now < 3100; now += 50) {
        if (!continuous.ready(now)) continue;
        continuous.submitted(now, continuous.choose(true));
        ++partials;
    }
    ok &= check(partials == 7, "A long hold must combine frames into seven partial updates in 2.1 seconds");
    ok &= check(!continuous.ready(3099) && continuous.ready(3100) &&
                continuous.choose(true) == RefreshAction::Partial,
                "The final movement frame must wait for the panel and remain drawable");
    continuous.submitted(3100, RefreshAction::Partial);
    ok &= check(continuous.choose(false) == RefreshAction::None, "Walking and idle must not request a flash");

    RefreshSchedule single_steps(0);
    for (uint64_t now = 1000; now <= 5000; now += 1000) {
        single_steps.submitted(now, RefreshAction::Partial);
    }
    ok &= check(single_steps.choose(false) == RefreshAction::None, "Separate steps must not request a flash");
    ok &= check(single_steps.ready(5300) && single_steps.choose(true) == RefreshAction::Partial,
                "A changed dialogue frame can use the next partial update");
    single_steps.request_clear();
    single_steps.defer(6000);
    ok &= check(!single_steps.ready(6099) && single_steps.ready(6100) &&
                single_steps.choose(true) == RefreshAction::Clear && single_steps.choose(false) == RefreshAction::Clear,
                "Explicit cleanup must survive a busy panel and take priority over changed content");
    single_steps.submitted(6100, RefreshAction::Clear);
    ok &= check(single_steps.choose(false) == RefreshAction::None &&
                single_steps.choose(true) == RefreshAction::Partial, "Manual cleanup is consumed once");
    return ok;
}

bool test_display_palettes() {
    Frame source{};
    const uint8_t levels[] = {0, 42, 43, 127, 128, 212, 213, 255};
    std::memcpy(source.data(), levels, sizeof levels);
    const Frame gray = display_frame(source, DisplayPalette::Gray4);
    const Frame mono = display_frame(source, DisplayPalette::Mono);
    const uint8_t gray_expected[] = {0, 0, 85, 85, 170, 170, 255, 255};
    const uint8_t mono_expected[] = {0, 0, 0, 0, 255, 255, 255, 255};
    return check(std::memcmp(gray.data(), gray_expected, sizeof levels) == 0 &&
                 std::memcmp(mono.data(), mono_expected, sizeof levels) == 0,
                 "The DU4 and DU palettes must only emit their supported gray levels");
}

bool test_game(const char* path, const char* save_origin) {
    std::vector<uint8_t> rom;
    std::string error;
    if (!read_rom(path, rom, error)) return check(false, error.c_str());
    std::vector<uint8_t> older_rom;
    if (save_origin && !read_rom(save_origin, older_rom, error)) return check(false, error.c_str());
    if (!check(boot(save_origin ? older_rom : rom), "Gambatte must load the ROM and expose 32 KiB of SRAM"))
        return false;
    advance(Button::None, 240);
    const Frame title = frame();
    Frame blank;
    blank.fill(title[0]);
    bool ok = check(title != blank, "The title screen must contain graphics");
    confirm();
    ok &= check(frame() != title, "A must open the save-slot selection");
    // Choose slot one, skip the scroll, accept a preset name and keep the default stats.
    for (int i = 0; i < 5 && std::memcmp(save_ram() + 1, "HERO", 4) != 0; ++i) confirm();
    const bool created = check(std::memcmp(save_ram() + 1, "HERO", 4) == 0,
                               "Creating a character must write a game save to slot one");
    Save saved;
    std::memcpy(saved.data(), save_ram(), saved.size());
    shutdown();
    if (!ok || !created || !check(boot(rom), "Gambatte must be able to restart")) return false;
    std::memcpy(save_ram(), saved.data(), saved.size());
    advance(Button::None, 240);
    const Frame continue_menu = frame();
    ok = check(continue_menu != title, "Restored SRAM must enable the Continue menu");
    confirm();
    ok &= check(frame() != continue_menu && std::memcmp(save_ram() + 1, "HERO", 4) == 0,
                "Continue must load the saved character");
    if (ok) ok = test_monk();
    shutdown();
    return ok;
}
}

int main(int argc, char** argv) {
    return (argc >= 2 ? test_game(argv[1], argc == 3 ? argv[2] : nullptr)
                      : test_bounds() && test_refresh_schedule() && test_display_palettes()) ? 0 : 1;
}
