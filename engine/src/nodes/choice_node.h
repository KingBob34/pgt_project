#ifndef CHOICE_NODE_H
#define CHOICE_NODE_H
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
#include "condition.h"
#include "nodes/node.h"

// What the player sees when an option's condition is not met
enum class UnmetDisplay
{
    Hide,   // leave the option out of the list entirely
    Show   // still list it, mark it as unavailable
};

// One selectable option of a ChoiceNode
struct ChoiceOption
{
    std::string pin;   // name of the flow output pin this option leaves through
    std::string text;   // label shown to the player

    // No condition at all means the option is always available
    std::optional<Condition> condition;
    UnmetDisplay whenUnmet = UnmetDisplay::Hide;
};

// Presents a list of options and waits for the player to pick one.
struct ChoiceNode : Node
{
    std::vector<ChoiceOption> options;
    [[nodiscard]] std::string typeName() const override
    {
        return "choice";
    }

    // One flow pin per option, so the pin list only exists once a story is loaded
    [[nodiscard]] std::vector<std::string> outputPins() const override
    {
        std::vector<std::string> pins;
        pins.reserve(options.size());
        for (const ChoiceOption& option: options)
        {
            pins.push_back(option.pin);
        }
        return pins;
    }

    [[nodiscard]] std::vector<const Condition*> conditions() const override
    {
        std::vector<const Condition*> found;
        for (const ChoiceOption& option : options)
        {
            if (option.condition.has_value())
            {
                found.push_back(&option.condition.value());
            }
        }
        return found;
    }

    [[nodiscard]] NodeResult execute(GameState& state) const override
    {
        NodeResult result;
        result.kind = NodeResult::Kind::Ask;
        for (const ChoiceOption& option : options)
        {
            bool available = true;
            if (option.condition.has_value())
            {
                available = evaluate(option.condition.value(), state);
            }
            if (!available && option.whenUnmet == UnmetDisplay::Hide) continue;

            result.options.push_back({option.text, available});
            result.pins.push_back(option.pin);
        }
        return result;
    }

    void readFields(const Value& json) override
    {
        const Value& entries = json.at("options");
        if (!entries.is_array() || entries.empty())
        {
            throw std::runtime_error("a choice node needs at least one option");
        }
        options.clear();
        options.reserve(entries.size());
        for (const Value& entry : entries)
        {
            ChoiceOption option;
            option.pin = entry.at("pin").get<std::string>();
            option.text = entry.at("text").get<std::string>();

            if (entry.contains("condition"))
            {
                option.condition = parseCondition(entry.at("condition"));
            }
            if (entry.contains("whenUnmet"))
            {
                const std::string whenUnmet = entry.at("whenUnmet").get<std::string>();
                if (whenUnmet == "hide")
                {
                    option.whenUnmet = UnmetDisplay::Hide;
                }
                else if (whenUnmet == "show")
                {
                    option.whenUnmet = UnmetDisplay::Show;
                }
                else
                {
                    throw std::runtime_error("unknown whenUnmet value: '" + whenUnmet + "'");
                }
            }
            options.push_back(option);
        }
    }
};

#endif //CHOICE_NODE_H
