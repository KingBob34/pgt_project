#include "game_state.h"

const Value* GameState::find(const std::string& name) const
{
    size_t dot = name.find('.');

    // The first segment is the variable's own name
    const auto it = variables.find(name.substr(0, dot));
    if (it == variables.end()) return nullptr;

    const Value* current = &it->second;
    // Every further segment steps one level down into an object
    while (dot != std::string::npos)
    {
        const size_t start = dot + 1;
        dot = name.find('.', start);
        const size_t length = (dot == std::string::npos) ? std::string::npos : dot - start;
        const std::string key = name.substr(start, length);
        if (!current->is_object() || !current->contains(key)) return nullptr;
        current = &current->at(key);
    }
    return current;
}
