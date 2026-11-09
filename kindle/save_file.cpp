#include "save_file.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

namespace golden {
namespace {
SaveStatus read_slot(const std::string& path, Journey& journey, uint32_t& generation) {
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return errno == ENOENT ? SaveStatus::Missing : SaveStatus::IoError;
    SaveBytes bytes(save_limit + 1);
    const auto size = std::fread(bytes.data(), 1, bytes.size(), file);
    const bool failed = std::ferror(file);
    const int closed = std::fclose(file);
    if (failed || closed) return SaveStatus::IoError;
    bytes.resize(size);
    return decode_save(bytes, journey, generation);
}

bool write_staged(const std::string& target, const SaveBytes& bytes) {
    const auto name = target + ".new.XXXXXX";
    std::vector<char> temporary(name.begin(), name.end());
    temporary.push_back('\0');
    const int fd = mkstemp(temporary.data());
    if (fd < 0) return false;
    std::size_t total = 0;
    while (total < bytes.size()) {
        const ssize_t result = ::write(fd, bytes.data() + total, bytes.size() - total);
        if (result < 0 && errno == EINTR) continue;
        if (result <= 0) break;
        total += static_cast<std::size_t>(result);
    }
    const bool synced = total == bytes.size() && fsync(fd) == 0;
    const bool closed = close(fd) == 0;
    if (synced && closed && std::rename(temporary.data(), target.c_str()) == 0) return true;
    unlink(temporary.data());
    return false;
}
}

SaveFile::SaveFile(std::string directory) : directory_(std::move(directory)) {}

std::string SaveFile::path(int slot) const {
    return directory_ + (slot == 0 ? "/journey.0" : "/journey.1");
}

SaveStatus SaveFile::load(Journey& journey) {
    generation_ = 0;
    last_slot_ = -1;
    write_allowed_ = true;
    SaveStatus error = SaveStatus::Missing;
    for (int slot = 0; slot < 2; ++slot) {
        Journey candidate;
        uint32_t generation = 0;
        const auto result = read_slot(path(slot), candidate, generation);
        if (result == SaveStatus::Loaded && generation > generation_) {
            journey = candidate;
            generation_ = generation;
            last_slot_ = slot;
        }
        if (result == SaveStatus::Unsupported || result == SaveStatus::IoError) {
            write_allowed_ = false;
            error = result;
        } else if (result == SaveStatus::Damaged && error == SaveStatus::Missing) {
            error = result;
        }
    }
    return last_slot_ >= 0 ? SaveStatus::Loaded : error;
}

bool SaveFile::write(const Journey& journey) {
    if (!write_allowed_ || generation_ == UINT32_MAX) return false;
    const auto bytes = encode_save(journey, generation_ + 1);
    const int slot = last_slot_ == 0 ? 1 : 0;
    if (bytes.empty() || !write_staged(path(slot), bytes)) return false;
    ++generation_;
    last_slot_ = slot;
    const int directory = open(directory_.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory >= 0) {
        fsync(directory);
        close(directory);
    }
    return true;
}
}
