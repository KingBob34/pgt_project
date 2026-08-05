#ifndef END_NODE_H
#define END_NODE_H
#include <string>
#include <vector>
#include "nodes/node.h"

// Terminates the story. Execution stops here.
struct EndNode : Node
{
    std::string outcome;   // author-defined label for this ending

    [[nodiscard]] std::string typeName() const override
    {
        return "end";
    }

    [[nodiscard]] std::vector<std::string> outputPins() const override
    {
        return {};
    }

    [[nodiscard]] NodeResult execute(GameState&) const override
    {
        NodeResult result;
        result.kind = NodeResult::Kind::Stop;
        result.outcome = outcome;
        return result;
    }

    void readFields(const Value& json) override
    {
        outcome = json.at("outcome").get<std::string>();
    }
};

#endif //END_NODE_H
