#include "fb.h"
#include "save_file.h"
#include "touch.h"
#include "view.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr int scale = 2;
constexpr uint64_t heartbeat_interval_ms = 2000;
constexpr uint64_t refresh_interval_ms = 220;
constexpr uint64_t first_repeat_ms = 400;
constexpr uint64_t repeat_ms = 280;
volatile std::sig_atomic_t interrupted = 0;

void stop(int) { interrupted = 1; }

bool heartbeat(const char* path, uint64_t timestamp) {
    std::FILE* file = std::fopen(path, "w");
    if (!file) return false;
    const bool wrote = std::fprintf(file, "%llu\n", static_cast<unsigned long long>(timestamp)) > 0;
    return std::fclose(file) == 0 && wrote;
}

uint64_t now_ms() {
    timespec time{};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<uint64_t>(time.tv_sec) * 1000 + time.tv_nsec / 1000000;
}

bool redraw(FbScreen& fb, int left, const golden::Frame& frame, golden::Rect rect) {
    const int row_bytes = rect.w / 8;
    std::vector<uint8_t> packed(static_cast<size_t>(row_bytes) * rect.h);
    for (int y = 0; y < rect.h; ++y) {
        const auto* source = frame.data() + (y + rect.y) * golden::view_stride + rect.x / 8;
        std::memcpy(packed.data() + y * row_bytes, source, row_bytes);
    }
    const int x = left + rect.x * scale;
    const int y = rect.y * scale;
    fb.blit_mono_1bpp(x, y, rect.w, rect.h, packed.data(), scale);
    return fb.refresh(x, y, rect.w * scale, rect.h * scale);
}

golden::Action action_at(const TouchPoint& point, int left) {
    if (!point.valid || point.x < left || point.x >= left + golden::view_width * scale)
        return golden::Action::None;
    return golden::hit_test({(point.x - left) / scale, point.y / scale});
}
}

int main(int argc, char** argv) {
    using namespace golden;
    const char* heartbeat_path = std::getenv("GOLDEN_HERO_HEARTBEAT");
    const char* save_directory = std::getenv("GOLDEN_HERO_SAVE_DIR");
    if (argc != 2 || std::strcmp(argv[1], "--protected-session") != 0 ||
        !heartbeat_path || !*heartbeat_path || !save_directory || !*save_directory) {
        std::fputs("Start Golden Hero from its KUAL menu.\n", stderr);
        return 2;
    }
    const pid_t launcher = getppid();
    if (launcher <= 1 || !heartbeat(heartbeat_path, now_ms())) return 3;
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);

    std::string error;
    FbScreen fb;
    if (!fb.open_fb(error)) {
        std::fprintf(stderr, "Framebuffer: %s\n", error.c_str());
        return 10;
    }
    if (fb.w != 758 || fb.h != view_height * scale || fb.depth != 8) {
        std::fputs("Expected PW1 758x1024 8-bit framebuffer.\n", stderr);
        fb.close_fb();
        return 10;
    }

    TouchInput touch;
    if (!touch.open_touch(error)) {
        std::fprintf(stderr, "Touchscreen: %s\n", error.c_str());
        fb.close_fb();
        return 11;
    }

    const int left = (fb.w - view_width * scale) / 2;
    const size_t visible_bytes = static_cast<size_t>(fb.stride) * fb.h;
    uint8_t* visible = fb.data + static_cast<size_t>(fb.stride) * fb.var.yoffset;
    const std::vector<uint8_t> backup(visible, visible + visible_bytes);
    Town town;
    SaveFile saves(save_directory);
    Journey loaded;
    const SaveStatus load_status = saves.load(loaded);
    if (load_status == SaveStatus::Loaded) town.resume(loaded);
    else if (load_status != SaveStatus::Missing) {
        std::fprintf(stderr, "Standalone save load status: %d\n", static_cast<int>(load_status));
        town.save_result(false);
    }
    Frame previous, frame;
    render(town, previous);
    fb.rect(0, 0, left, fb.h, 255);
    fb.rect(left + view_width * scale, 0, left, fb.h, 255);
    bool display_ok = redraw(fb, left, previous, {0, 0, view_width, view_height});
    uint64_t next_refresh = now_ms() + refresh_interval_ms;
    uint64_t next_heartbeat = 0;
    uint64_t next_repeat = 0;
    Action active = Action::None;
    bool pending = false;
    bool exit_requested = false;
    std::fprintf(stderr, "Standalone game ready; EPDC=%s.\n",
                 display_ok ? "OK" : "FAILED");

    while (display_ok && !exit_requested && !interrupted) {
        TouchPoint point;
        const uint64_t now = now_ms();
        if (now >= next_heartbeat) {
            if (getppid() != launcher || !heartbeat(heartbeat_path, now)) {
                std::fputs("Lost the fullscreen launcher heartbeat.\n", stderr);
                break;
            }
            next_heartbeat = now + heartbeat_interval_ms;
        }
        Direction direction;
        for (int events = 0; events < 16 && touch.poll_point(fb.w, fb.h, point); ++events) {
            const Action hit = action_at(point, left);
            if (point.pressed) {
                active = hit;
                if (direction_for(active, direction)) {
                    pending |= town.move(direction);
                    next_repeat = now + first_repeat_ms;
                }
            } else if (point.moved && hit != active) {
                active = Action::None;
            }
            if (point.released) {
                if (hit == active) {
                    if (active == Action::Confirm) pending |= town.confirm();
                    if (active == Action::Back) pending |= town.back();
                    if (active == Action::Menu) pending |= town.menu();
                    if (active == Action::Exit) exit_requested = true;
                }
                active = Action::None;
            }
            if (exit_requested || interrupted) break;
        }
        if (exit_requested || interrupted) break;

        if (direction_for(active, direction) && now >= next_repeat) {
            pending |= town.move(direction);
            next_repeat = now + repeat_ms;
        }
        if (town.take_save_request() && town.can_save()) {
            town.save_result(saves.write(town.journey()));
            pending = true;
        }
        if (pending && now >= next_refresh) {
            render(town, frame);
            const Rect dirty = changed_region(previous, frame);
            if (dirty.w > 0) {
                display_ok = redraw(fb, left, frame, dirty);
                previous = frame;
                next_refresh = now + refresh_interval_ms;
            }
            pending = false;
        }
        pollfd event{touch.fd, POLLIN, 0};
        const int result = poll(&event, 1, 20);
        if ((result < 0 && !interrupted) || (result > 0 && (event.revents & (POLLERR | POLLHUP | POLLNVAL)))) {
            std::fputs("Touchscreen poll failed.\n", stderr);
            display_ok = false;
            break;
        }
    }

    touch.close_touch();
    std::memcpy(visible, backup.data(), visible_bytes);
    const bool restored = fb.refresh(0, 0, fb.w, fb.h, true);
    fb.close_fb();
    if ((exit_requested || interrupted) && town.can_save() && !saves.write(town.journey()))
        std::fputs("Unable to save the final checkpoint.\n", stderr);
    std::fprintf(stderr, "Framebuffer restored: %s; exit=%s\n", restored ? "OK" : "FAILED",
                 exit_requested ? "button" : interrupted ? "signal" : "device error");
    return display_ok && restored ? 0 : 12;
}
