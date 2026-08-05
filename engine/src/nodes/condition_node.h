#ifndef CONDITION_NODE_H
#define CONDITION_NODE_H
#include <string>
#include <vector>
#include "condition.h"
#include "nodes/node.h"

// Branches on the current game state.
struct ConditionNode : Node
{
    Condition condition;   // evaluated against the current variables

    [[nodiscard]] std::string typeName() const override
    {
        return "condition";
    }

    [[nodiscard]] std::vector<std::string> outputPins() const override
    {
        return {"true", "false"};
    }

    [[nodiscard]] std::vector<const Condition*> conditions() const override
    {
        return {&condition};
    }

    [[nodiscard]] NodeResult execute(GameState& state) const override
    {
        NodeResult result;
        result.kind = NodeResult::Kind::Continue;
        result.pin = evaluate(condition, state) ? "true" : "false";
        return result;
    }

    void readFields(const Value& json) override
    {
        condition = parseCondition(json.at("condition"));
    }
};

#endif //CONDITION_NODE_H
