#ifndef START_NODE_H
#define START_NODE_H
#include <string>
#include <vector>
#include "nodes/node.h"

// Entry point of the graph.
// Holds no data of its own.
// It exists so that execution has a well-defined place to begin.
struct StartNode : Node
{
    [[nodiscard]] std::string typeName() const override
    {
        return "start";
    }

    [[nodiscard]] std::vector<std::string> outputPins() const override
    {
        return {"out"};
    }

    [[nodiscard]] NodeResult execute(GameState&) const override
    {
        NodeResult result;
        result.kind = NodeResult::Kind::Continue;
        result.pin = "out";
        return result;
    }

    void readFields(const Value&) override {}
};

#endif //START_NODE_H
