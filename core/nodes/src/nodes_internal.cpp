#include "nodes_internal.h"

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

    Value defaultColor()
    {
        Value color = Value::object();
        color["r"] = 1.0;
        color["g"] = 1.0;
        color["b"] = 1.0;
        color["a"] = 1.0;
        return color;
    }

    void reportError(ExecutionContext& context, const std::string& node, const std::string& detail)
    {
        Value details = Value::object();
        details["node"] = node;
        details["detail"] = detail;

        context.host().command("error", details);
    }
}
