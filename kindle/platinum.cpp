#include "fb.h"
#include "platinum_core.h"
#include "touch.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <time.h>
#include <unistd.h>

namespace {
using namespace platinum;
constexpr int screen_width = 758;
constexpr int screen_height = 1024;
constexpr int scale = 4;
constexpr int left = (screen_width - width * scale) / 2;
constexpr int top = 24;
constexpr uint64_t frame_ns = 16742706;
constexpr uint64_t save_ns = 5000000000;
constexpr uint64_t heartbeat_ns = 2000000000;
volatile std::sig_atomic_t interrupted = 0;

struct Control { Button button; Region bounds; const char* label; };
constexpr Control controls[] = {
    {Button::Up,     {152, 654, 112, 82}, "UP"},
    {Button::Left,   { 32, 744, 112, 82}, "LEFT"},
    {Button::Right,  {272, 744, 112, 82}, "RIGHT"},
    {Button::Down,   {152, 834, 112, 82}, "DOWN"},
    {Button::B,      {474, 754, 106, 96}, "B"},
    {Button::A,      {606, 696, 106, 96}, "A"},
    {Button::Select, { 32, 942, 158, 62}, "SELECT"},
    {Button::Start,  {205, 942, 158, 62}, "START"},
    {Button::Clear,  {394, 942, 144, 62}, "CLEAN"},
    {Button::Exit,   {568, 942, 144, 62}, "EXIT"},
};

// Five-bit rows, top to bottom, for the labels on the virtual controller.
constexpr char letters[] = "ABCDEFGHILNOPRSTUWX";
constexpr uint8_t glyphs[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16}, {14,17,16,23,17,17,15},
    {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14}, {16,16,16,16,16,16,31},
    {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {30,17,17,30,20,18,17}, {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
};
static_assert(sizeof(letters) - 1 == sizeof(glyphs) / sizeof(glyphs[0]), "Controller font mismatch");

void stop(int) { interrupted = 1; }

uint64_t now_ns() {
    timespec time{};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<uint64_t>(time.tv_sec) * 1000000000 + time.tv_nsec;
}

Button hit_test(const TouchPoint& point) {
    if (!point.valid) return Button::None;
    for (const auto& control : controls) {
        const auto r = control.bounds;
        if (point.x >= r.x && point.x < r.x + r.w && point.y >= r.y && point.y < r.y + r.h)
            return control.button;
    }
    return Button::None;
}

void draw_controls(FbScreen& fb) {
    fb.border(left - 2, top - 2, width * scale + 4, height * scale + 4, 0, 2);
    for (const auto& control : controls) {
        const auto r = control.bounds;
        fb.border(r.x, r.y, r.w, r.h, 0);
        constexpr int dot = 3;
        const int y = r.y + (r.h - 7 * dot) / 2;
        int x = r.x + (r.w - (6 * static_cast<int>(std::strlen(control.label)) - 1) * dot) / 2;
        for (const char* label = control.label; *label; ++label, x += 6 * dot) {
            const char* letter = std::strchr(letters, *label);
            if (!letter) continue;
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if (glyphs[letter - letters][row] & (16 >> col))
                        fb.rect(x + col * dot, y + row * dot, dot, dot, 0);
                }
            }
        }
    }
}

void draw_frame(FbScreen& fb, const Frame& image, Region dirty) {
    uint8_t row[width * scale];
    const bool invert = fb.var.grayscale == 2;
    for (int y = dirty.y; y < dirty.y + dirty.h; ++y) {
        for (int x = 0; x < dirty.w; ++x) {
            const uint8_t gray = image[y * width + dirty.x + x];
            std::memset(row + x * scale, invert ? 255 - gray : gray, scale);
        }
        uint8_t* destination = fb.data + (fb.var.yoffset + top + y * scale) * fb.stride
                               + fb.var.xoffset + left + dirty.x * scale;
        for (int repeat = 0; repeat < scale; ++repeat, destination += fb.stride)
            std::memcpy(destination, row, dirty.w * scale);
    }
}

bool read_save(const char* path, Save& saved) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return errno == ENOENT;
    const bool valid = std::fread(saved.data(), 1, saved.size(), file) == saved.size()
                       && std::fgetc(file) == EOF && !std::ferror(file);
    std::fclose(file);
    if (valid) std::memcpy(save_ram(), saved.data(), saved.size());
    return valid;
}

bool write_save(const char* path, Save& saved) {
    const uint8_t* ram = save_ram();
    if (std::memcmp(ram, saved.data(), saved.size()) == 0) return true;
    std::string temporary = std::string(path) + ".tmp.XXXXXX";
    std::vector<char> name(temporary.begin(), temporary.end());
    name.push_back('\0');
    const int fd = mkstemp(name.data());
    if (fd < 0) return false;
    FILE* file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(name.data());
        return false;
    }
    const bool written = std::fwrite(ram, 1, saved.size(), file) == saved.size()
                         && std::fflush(file) == 0 && fsync(fd) == 0;
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed || std::rename(name.data(), path) != 0) {
        unlink(name.data());
        return false;
    }
    std::memcpy(saved.data(), ram, saved.size());
    return true;
}

bool play(FbScreen& fb, TouchInput& touch, const char* save_path, Save& saved, const char* heartbeat_path) {
    const size_t visible_bytes = static_cast<size_t>(fb.stride) * fb.h;
    uint8_t* visible = fb.data + static_cast<size_t>(fb.stride) * fb.var.yoffset;
    const std::vector<uint8_t> backup(visible, visible + visible_bytes);
    fb.rect(0, 0, fb.w, fb.h, 255);
    draw_controls(fb);
    run(Button::None);
    DisplayPalette palette = DisplayPalette::Gray4;
    Frame displayed = display_frame(frame(), palette);
    draw_frame(fb, displayed, {0, 0, width, height});
    bool ok = fb.refresh(0, 0, fb.w, fb.h, true);
    const uint64_t start = now_ns();
    uint64_t next_frame = start + frame_ns;
    uint64_t next_save = start + save_ns;
    uint64_t next_heartbeat = start;
    unsigned heartbeat_sequence = 0;
    RefreshSchedule refresh(start / 1000000);
    unsigned partials = 0, clears = 0, busy_total = 0, busy_streak = 0;
    Button held = Button::None;
    Button tap = Button::None;
    int tap_frames = 0;
    Button armed = Button::None;
    bool exit_requested = false;

    std::fputs("Starting GBC session v1.0.0 (DU4, heartbeat monitored).\n", stderr);
    while (ok && !interrupted && !exit_requested) {
        TouchPoint point;
        for (int i = 0; i < 16 && touch.poll_point(fb.w, fb.h, point); ++i) {
            const Button hit = hit_test(point);
            const bool local = hit == Button::Exit || hit == Button::Clear;
            if (point.pressed) {
                armed = local ? hit : Button::None;
                held = local ? Button::None : hit;
                tap = held;
                tap_frames = held == Button::None ? 0 : 2;
            } else if (point.moved) {
                if (hit != armed) armed = Button::None;
                held = local ? Button::None : hit;
            } else if (point.released) {
                if (armed == hit && hit == Button::Clear) refresh.request_clear();
                exit_requested = armed == hit && hit == Button::Exit;
                armed = Button::None;
                held = Button::None;
            }
            if (exit_requested) break;
        }
        if (exit_requested || interrupted) break;

        const uint64_t now = now_ns();
        const uint64_t now_ms = now / 1000000;
        if (now >= next_heartbeat) {
            FILE* heartbeat = std::fopen(heartbeat_path, "wb");
            const bool written = heartbeat && std::fprintf(heartbeat, "%u\n", ++heartbeat_sequence) > 0;
            const bool closed = heartbeat && std::fclose(heartbeat) == 0;
            if (!written || !closed) {
                std::fputs("Could not update session heartbeat.\n", stderr);
                ok = false;
                break;
            }
            next_heartbeat = now + heartbeat_ns;
        }
        const bool frame_ready = now >= next_frame;
        if (frame_ready) {
            const Button button = tap_frames > 0 ? tap : held;
            run(button);
            if (tap_frames > 0) --tap_frames;
            next_frame += frame_ns;
            if (next_frame <= now) next_frame = now + frame_ns;
        }
        if (frame_ready && refresh.ready(now_ms)) {
            const Frame current = display_frame(frame(), palette);
            const Region dirty = changed_region(displayed, current);
            const RefreshAction action = refresh.choose(dirty.w > 0);
            if (action != RefreshAction::None) {
                const bool clear = action == RefreshAction::Clear;
                const Region region = clear ? Region{0, 0, width, height} : dirty;
                const FbGameUpdate mode = clear ? FbGameUpdate::Clear :
                    palette == DisplayPalette::Mono ? FbGameUpdate::Mono : FbGameUpdate::Gray4;
                draw_frame(fb, current, region);
                const FbRefreshResult result = fb.refresh_game(left + region.x * scale,
                    top + region.y * scale, region.w * scale, region.h * scale, mode);
                if (result == FbRefreshResult::Submitted) {
                    if (clear) ++clears;
                    else ++partials;
                    displayed = current;
                    busy_streak = 0;
                    refresh.submitted(now_ms, action);
                } else if (result == FbRefreshResult::Unsupported) {
                    draw_frame(fb, displayed, region);
                    palette = DisplayPalette::Mono;
                    busy_streak = 0;
                    std::fputs("DU4 rejected by EPDC; switching to DU monochrome.\n", stderr);
                } else if (result == FbRefreshResult::Busy) {
                    draw_frame(fb, displayed, region);
                    if (++busy_streak == 1) std::fputs("EPDC busy; deferring redraw.\n", stderr);
                    ++busy_total;
                    refresh.defer(now_ms);
                    if (busy_streak >= 20) {
                        std::fputs("EPDC remained busy after 20 attempts.\n", stderr);
                        ok = false;
                    }
                } else {
                    ok = false;
                }
            } else {
                busy_streak = 0;
            }
        }
        if (now >= next_save) {
            if (!write_save(save_path, saved)) std::fputs("Could not write SRAM; will retry.\n", stderr);
            next_save = now + save_ns;
        }
        pollfd event{touch.fd, POLLIN, 0};
        const int result = poll(&event, 1, 4);
        if ((result < 0 && !interrupted) || (event.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            std::fputs("Touchscreen poll failed.\n", stderr);
            ok = false;
        }
    }

    std::memcpy(visible, backup.data(), visible_bytes);
    const bool restored = fb.refresh(0, 0, fb.w, fb.h, true);
    std::fprintf(stderr, "Framebuffer restored: %s; exit=%s; elapsed_ms=%llu; partials=%u; clears=%u; busy=%u; waveform=%s\n",
                 restored ? "OK" : "FAILED",
                 exit_requested ? "button" : interrupted ? "signal" : "device error",
                 static_cast<unsigned long long>((now_ns() - start) / 1000000),
                 partials, clears, busy_total, palette == DisplayPalette::Mono ? "DU" : "DU4");
    const bool saved_ok = write_save(save_path, saved);
    if (!saved_ok) std::fputs("Could not write SRAM on exit.\n", stderr);
    return ok && restored && saved_ok;
}
}

int main(int argc, char** argv) {
    const bool check = argc == 3 && std::strcmp(argv[1], "--verify-rom") == 0;
    if (!check && (argc != 5 || std::strcmp(argv[1], "--protected-session") != 0)) {
        std::fputs("Start Platinum Hero from its KUAL menu.\n", stderr);
        return 2;
    }
    std::string error;
    std::vector<uint8_t> rom;
    if (!read_rom(argv[2], rom, error) || !boot(rom)) {
        std::fprintf(stderr, "%s\n", error.empty() ? "Gambatte could not start the GBC ROM." : error.c_str());
        return 3;
    }
    if (check) {
        shutdown();
        return 0;
    }

    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    Save saved;
    std::memcpy(saved.data(), save_ram(), saved.size());
    if (!read_save(argv[3], saved)) {
        std::fputs("Could not read the existing 32 KiB save. The file was preserved.\n", stderr);
        shutdown();
        return 4;
    }
    FbScreen fb;
    TouchInput touch;
    bool ok = false;
    if (!fb.open_fb(error) || fb.w != screen_width || fb.h != screen_height || fb.depth != 8 ||
        (fb.var.grayscale != 1 && fb.var.grayscale != 2)) {
        std::fprintf(stderr, "Framebuffer: %s\n", error.empty() ? "Expected PW1 758x1024 8-bit grayscale." : error.c_str());
    } else if (!touch.open_touch(error)) {
        std::fprintf(stderr, "Touchscreen: %s\n", error.c_str());
    } else if (!interrupted) {
        ok = play(fb, touch, argv[3], saved, argv[4]);
    }
    touch.close_touch();
    fb.close_fb();
    shutdown();
    return ok ? 0 : 5;
}
