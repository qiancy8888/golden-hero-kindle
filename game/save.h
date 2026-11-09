#pragma once

#include "town.h"

#include <cstdint>
#include <vector>

namespace golden {
using SaveBytes = std::vector<uint8_t>;
constexpr std::size_t save_limit = 1024;

enum class SaveStatus { Missing, Loaded, Damaged, Unsupported, IoError };

SaveBytes encode_save(const Journey& journey, uint32_t generation);
SaveStatus decode_save(const SaveBytes& bytes, Journey& journey, uint32_t& generation);
}
