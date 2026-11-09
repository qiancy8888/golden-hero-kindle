#include "save_file.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace golden;

#define CHECK(condition) do { \
    if (!(condition)) { std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition); std::exit(1); } \
} while (false)

namespace {
void replace(const std::string& filename, const SaveBytes& bytes) {
    std::FILE* file = std::fopen(filename.c_str(), "wb");
    CHECK(file);
    CHECK(std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size());
    CHECK(std::fclose(file) == 0);
}
}

int main() {
    char dirname[] = "/tmp/golden-save-test.XXXXXX";
    CHECK(mkdtemp(dirname));
    const std::string dir = dirname, first = dir + "/journey.0", second = dir + "/journey.1";
    Journey hero;
    hero.hero = new_hero(Hero{});
    SaveFile saves(dir);
    Journey loaded;
    CHECK(saves.load(loaded) == SaveStatus::Missing);
    CHECK(saves.write(hero));
    hero.hero.coins = 99;
    CHECK(saves.write(hero));
    SaveFile reopened(dir);
    CHECK(reopened.load(loaded) == SaveStatus::Loaded && loaded.hero.coins == 99);
    replace(second, {0,1,2});
    CHECK(reopened.load(loaded) == SaveStatus::Loaded && loaded.hero.coins == 30);
    CHECK(reopened.write(hero));
    auto future = encode_save(hero, 3);
    CHECK(!future.empty());
    future[4] = 2;
    replace(first, future);
    CHECK(reopened.load(loaded) == SaveStatus::Loaded && loaded.hero.coins == 99);
    CHECK(!reopened.write(hero));
    CHECK(std::remove(second.c_str()) == 0);
    CHECK(reopened.load(loaded) == SaveStatus::Unsupported);
    CHECK(!reopened.write(hero));
    CHECK(std::remove(first.c_str()) == 0);
    CHECK(rmdir(dirname) == 0);
    std::puts("Standalone save recovery checks passed.");
}
