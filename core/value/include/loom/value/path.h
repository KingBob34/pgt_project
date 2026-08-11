#ifndef LOOM_VALUE_PATH_H
#define LOOM_VALUE_PATH_H
#include <string>
#include <vector>
#include <cstddef>
#include "loom/value/value.h"

namespace loom
{
    // "pets.cat.hunger" -> { "pets", "cat", "hunger" }
    std::vector<std::string> splitPath(const std::string& path);

    // Steps into nested objects one segment at a time, starting at segments[from].
    // Returns nullptr as soon as a segment is missing or its holder is not an object.
    const Value* descend(const Value& root,
                         const std::vector<std::string>& segments,
                         std::size_t from = 0);
}

#endif //LOOM_VALUE_PATH_H
