#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace platinum {
constexpr int width = 160;
constexpr int height = 144;
constexpr size_t save_size = 32 * 1024;
using Frame = std::array<uint8_t, width * height>;
using Save = std::array<uint8_t, save_size>;

struct Region { int x, y, w, h; };
enum class Button { None, Up, Down, Left, Right, A, B, Start, Select, Clear, Exit };
enum class RefreshAction { None, Partial, Clear };
enum class DisplayPalette { Gray4, Mono };

class RefreshSchedule {
public:
    explicit RefreshSchedule(uint64_t start_ms) : last_update(start_ms) {}
    void request_clear();
    bool ready(uint64_t now_ms) const;
    RefreshAction choose(bool changed) const;
    void submitted(uint64_t now_ms, RefreshAction action);
    void defer(uint64_t now_ms);

private:
    uint64_t last_update;
    uint64_t retry_at = 0;
    bool clear_pending = false;
};

bool read_rom(const char* path, std::vector<uint8_t>& rom, std::string& error);
bool boot(const std::vector<uint8_t>& rom);
void shutdown();
void run(Button button);
const Frame& frame();
Frame display_frame(const Frame& source, DisplayPalette palette);
uint8_t* save_ram();
Region changed_region(const Frame& previous, const Frame& current);
}
