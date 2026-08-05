#ifndef NARRATIVE_NODE_H
#define NARRATIVE_NODE_H
#include <string>
#include <vector>
#include "nodes/node.h"

// Shows one block of prose to the player.
struct NarrativeNode : Node
{
    std::string text;   // the prose shown to the player

    [[nodiscard]] std::string typeName() const override
    {
        return "narrative";
    }

    [[nodiscard]] std::vector<std::string> outputPins() const override
    {
        return {"out"};
    }

    [[nodiscard]] NodeResult execute(GameState&) const override
    {
        NodeResult result;
        result.kind = NodeResult::Kind::Show;
        result.text = text;
        result.pin = "out";
        return result;
    }

    void readFields(const Value& json) override
    {
        text = json.at("text").get<std::string>();
    }
};

#endif //NARRATIVE_NODE_H
