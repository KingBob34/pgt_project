#include "loom/graph/pin.h"

namespace loom
{
    bool isCompatible(const std::string& from, const std::string& to)
    {
        if (from == PinType::Flow || to == PinType::Flow) return from == to;
        if (from == PinType::Any || to == PinType::Any) return true;

        return from == to;
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

        return type;
    }
}
