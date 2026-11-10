#include "platinum_core.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <libretro.h>

namespace platinum {
namespace {
constexpr uint64_t update_interval_ms = 300;
constexpr uint64_t retry_ms = 100;
Frame latest{};
Button pressed = Button::None;

bool environment(unsigned command, void* data) {
    if (command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_RGB565;
    if (command == RETRO_ENVIRONMENT_GET_CAN_DUPE) {
        *static_cast<bool*>(data) = true;
        return true;
    }
    return false;
}

void video(const void* pixels, unsigned w, unsigned h, size_t pitch) {
    if (!pixels || w != width || h != height || pitch < width * sizeof(uint16_t)) return;
    const auto* bytes = static_cast<const uint8_t*>(pixels);
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            uint16_t rgb;
            std::memcpy(&rgb, bytes + y * pitch + x * sizeof(rgb), sizeof(rgb));
            const unsigned red = ((rgb >> 11) & 31) * 255 / 31;
            const unsigned green = ((rgb >> 5) & 63) * 255 / 63;
            const unsigned blue = (rgb & 31) * 255 / 31;
            latest[y * width + x] = (77 * red + 150 * green + 29 * blue) >> 8;
        }
    }
}

int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (port != 0 || device != RETRO_DEVICE_JOYPAD || index != 0) return 0;
    switch (pressed) {
        case Button::Up: return id == RETRO_DEVICE_ID_JOYPAD_UP;
        case Button::Down: return id == RETRO_DEVICE_ID_JOYPAD_DOWN;
        case Button::Left: return id == RETRO_DEVICE_ID_JOYPAD_LEFT;
        case Button::Right: return id == RETRO_DEVICE_ID_JOYPAD_RIGHT;
        case Button::A: return id == RETRO_DEVICE_ID_JOYPAD_A;
        case Button::B: return id == RETRO_DEVICE_ID_JOYPAD_B;
        case Button::Start: return id == RETRO_DEVICE_ID_JOYPAD_START;
        case Button::Select: return id == RETRO_DEVICE_ID_JOYPAD_SELECT;
        default: return 0;
    }
}
}

bool read_rom(const char* path, std::vector<uint8_t>& rom, std::string& error) {
    constexpr size_t rom_size = 1024 * 1024;
    FILE* file = std::fopen(path, "rb");
    if (!file) {
        error = std::string("Cannot open ROM: ") + path;
        return false;
    }
    rom.resize(rom_size);
    const size_t count = std::fread(rom.data(), 1, rom.size(), file);
    const bool correct_size = count == rom.size() && std::fgetc(file) == EOF && !std::ferror(file);
    std::fclose(file);
    if (!correct_size) {
        error = "Expected the 1 MiB GMUD_GB ROM from the documented build.";
        return false;
    }
    uint8_t checksum = 0;
    for (size_t i = 0x134; i <= 0x14c; ++i) checksum = checksum - rom[i] - 1;
    if (std::memcmp(rom.data() + 0x134, "HEROTAN", 7) != 0 || rom[0x143] != 0xc0 ||
        rom[0x147] != 0x1b || rom[0x148] != 0x05 || rom[0x149] != 0x03 || checksum != rom[0x14d]) {
        error = "Invalid HEROTAN ROM header. Expected GBC, MBC5 with battery and 32 KiB RAM.";
        return false;
    }
    return true;
}

bool boot(const std::vector<uint8_t>& rom) {
    latest.fill(255);
    pressed = Button::None;
    retro_set_environment(environment);
    retro_set_video_refresh(video);
    retro_set_audio_sample([](int16_t, int16_t) {});
    retro_set_audio_sample_batch([](const int16_t*, size_t count) { return count; });
    retro_set_input_poll([] {});
    retro_set_input_state(input);
    retro_init();
    const retro_game_info game{nullptr, rom.data(), rom.size(), nullptr};
    if (!retro_load_game(&game)) {
        retro_deinit();
        return false;
    }
    if (retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) != save_size || !save_ram()) {
        shutdown();
        return false;
    }
    return true;
}

void shutdown() {
    retro_unload_game();
    retro_deinit();
}

void run(Button button) {
    pressed = button;
    retro_run();
}

const Frame& frame() { return latest; }

Frame display_frame(const Frame& source, DisplayPalette palette) {
    Frame result;
    if (palette == DisplayPalette::Mono) {
        std::transform(source.begin(), source.end(), result.begin(),
                       [](uint8_t gray) { return gray < 128 ? 0 : 255; });
    } else {
        // DU4 accepts black, 0x55, 0xAA, and white.
        std::transform(source.begin(), source.end(), result.begin(),
                       [](uint8_t gray) { return ((gray + 42) / 85) * 85; });
    }
    return result;
}

uint8_t* save_ram() {
    return static_cast<uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
}

bool RefreshSchedule::ready(uint64_t now) const {
    return now >= retry_at && now - last_update >= update_interval_ms;
}

void RefreshSchedule::request_clear() {
    clear_pending = true;
}

RefreshAction RefreshSchedule::choose(bool changed) const {
    if (clear_pending) return RefreshAction::Clear;
    return changed ? RefreshAction::Partial : RefreshAction::None;
}

void RefreshSchedule::submitted(uint64_t now, RefreshAction action) {
    last_update = now;
    retry_at = 0;
    if (action == RefreshAction::Clear) clear_pending = false;
}

void RefreshSchedule::defer(uint64_t now) { retry_at = now + retry_ms; }

Region changed_region(const Frame& previous, const Frame& current) {
    int left = width, top = height, right = -1, bottom = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t index = y * width + x;
            if (previous[index] == current[index]) continue;
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = y;
        }
    }
    if (right < 0) return {0, 0, 0, 0};
    return {left, top, right - left + 1, bottom - top + 1};
}
}
