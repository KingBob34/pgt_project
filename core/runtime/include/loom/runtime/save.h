#ifndef LOOM_RUNTIME_SAVE_H
#define LOOM_RUNTIME_SAVE_H
#include <string>

#include "loom/runtime/interpreter.h"

namespace loom
{
    // Save files carry their own version, separate from the graph format's.
    inline constexpr int kSaveVersion = 1;

    Value writeSave(const SaveState& state);

    // On failure returns false and describes what was wrong.
    bool readSave(const Value& document, SaveState& out, std::string& error);
}

#endif //LOOM_RUNTIME_SAVE_H
