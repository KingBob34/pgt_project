#ifndef GAME_STATE_H
#define GAME_STATE_H
#include <map>
#include <string>
#include "value.h"

// Everything that changes while a story is being played
struct GameState
{
    // the live variable table
    std::map<std::string, Value> variables;

    // where the player currently is
    int currentNodeId = 0;

    // Looks up a variable
    [[nodiscard]] const Value* find(const std::string& name) const;
};

#endif //GAME_STATE_H
