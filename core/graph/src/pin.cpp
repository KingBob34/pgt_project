#include "loom/graph/pin.h"

namespace loom
{
    bool isCompatible(const std::string& from, const std::string& to)
    {
        if (from == PinType::Flow || to == PinType::Flow) return from == to;

        // Both tested before Any, which would otherwise let anything reach one.
        if (to == PinType::VariableName) return false;
        if (from == PinType::Unset || to == PinType::Unset) return false;

        // Any takes anything in and hands it on only to another Any.
        if (from == PinType::Any) return to == PinType::Any;
        if (to == PinType::Any) return true;

        return from == to;
    }

    bool canHold(const std::string& pinType, const std::string& valueType)
    {
        // The chosen variable is stored as the text of its name, and a colour
        // as an object of channels.
        if (pinType == PinType::VariableName) return valueType == PinType::String;
        if (pinType == PinType::Color) return valueType == PinType::Any;

        return isCompatible(valueType, pinType);
    }

    std::string pinTypeLabel(const std::string& type)
    {
        if (type == PinType::Flow) return "Flow";
        if (type == PinType::Any) return "Any";
        if (type == PinType::Bool) return "Bool";
        if (type == PinType::Int) return "Integer";
        if (type == PinType::Float) return "Float";
        if (type == PinType::String) return "String";
        if (type == PinType::Color) return "Color";
        if (type == PinType::VariableName) return "Variable";
        if (type == PinType::Unset) return "None";

        return type;
    }
}
