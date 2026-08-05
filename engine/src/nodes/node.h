#ifndef NODE_H
#define NODE_H
#include <string>
#include <vector>
#include "value.h"
#include "condition.h"
#include "game_state.h"
#include "node_result.h"

// Canvas coordinates of a node.
// The editor uses these to restore the layout
struct Position
{
    double x = 0.0;
    double y = 0.0;
};

// Base class of every node type.
// It holds only what every node has.
// Each concrete type adds its own fields in its own header.
struct Node
{
    virtual ~Node() = default;

    // the type id as written in the story file, e.g. "narrative"
    [[nodiscard]] virtual std::string typeName() const = 0;

    // Names of this node's outgoing flow pins, in display order
    [[nodiscard]] virtual std::vector<std::string> outputPins() const = 0;

    // Every condition expression this node evaluated.
    // Empty for nodes that never test game state
    [[nodiscard]] virtual std::vector<const Condition*> conditions() const {return {};}

    // Reads this node's type-specific fields out of its JSON object
    virtual void readFields(const Value& json) = 0;

    // Performs this node and says what should happen next
    [[nodiscard]] virtual NodeResult execute(GameState& state) const = 0;

    int id = 0;   // unique within the story; assigned by the editor
    Position position;
};

#endif //NODE_H
