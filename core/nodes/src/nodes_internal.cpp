#include "nodes_internal.h"

#include "loom/graph/prose.h"

#include <utility>

namespace loom
{
    PinSpec flowIn(std::string name, std::string label)
    {
        return { std::move(name), std::move(label), PinDirection::Input, PinType::Flow, Value() };
    }

    PinSpec flowOut(std::string name, std::string label)
    {
        return { std::move(name), std::move(label), PinDirection::Output, PinType::Flow, Value() };
    }

    PinSpec dataIn(std::string name, std::string label, std::string type, Value defaultValue)
    {
        return { std::move(name), std::move(label), PinDirection::Input,
                 std::move(type), std::move(defaultValue) };
    }

    PinSpec dataOut(std::string name, std::string label, std::string type, Value defaultValue)
    {
        return { std::move(name), std::move(label), PinDirection::Output,
                 std::move(type), std::move(defaultValue) };
    }

    PinSpec labelTextIn(std::string name, std::string label, Value defaultValue)
    {
        return { std::move(name), std::move(label), PinDirection::Input,
                 PinType::String, std::move(defaultValue), TextShape::Label };
    }

    PinSpec longTextIn(std::string name, std::string label, Value defaultValue)
    {
        return { std::move(name), std::move(label), PinDirection::Input,
                 PinType::String, std::move(defaultValue), TextShape::Passage };
    }

    PinSpec variableIn(std::string name, std::string label)
    {
        return { std::move(name), std::move(label), PinDirection::Input,
                 PinType::VariableName, Value("") };
    }

    PinSpec followsIn(std::string name, std::string label, std::string follows)
    {
        return { std::move(name), std::move(label), PinDirection::Input,
                 PinType::Unset, Value(), TextShape::Line, std::move(follows) };
    }

    PinSpec followsOut(std::string name, std::string label, std::string follows)
    {
        return { std::move(name), std::move(label), PinDirection::Output,
                 PinType::Unset, Value(), TextShape::Line, std::move(follows) };
    }

    PinSpec sizeIn(std::string name, int start)
    {
        PinSpec pin = dataIn(std::move(name), "", PinType::Int, Value(start));
        pin.hidden = true;

        return pin;
    }

    PinSpec proseIn(std::string name, std::string label)
    {
        PinSpec pin = dataIn(std::move(name), std::move(label), PinType::Prose,
                             prose::fromPlain(""));
        pin.textShape = TextShape::Passage;

        return pin;
    }

    void reportError(ExecutionContext& context, const std::string& node, const std::string& detail)
    {
        Value details = Value::object();
        details["node"] = node;
        details["detail"] = detail;

        context.host().command("error", details);
    }
}
