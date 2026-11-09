#pragma once

#include "save.h"

#include <string>

namespace golden {
class SaveFile {
public:
    explicit SaveFile(std::string directory);
    SaveStatus load(Journey& journey);
    bool write(const Journey& journey);

private:
    std::string path(int slot) const;

    std::string directory_;
    uint32_t generation_ = 0;
    int last_slot_ = -1;
    bool write_allowed_ = false;
};
}
