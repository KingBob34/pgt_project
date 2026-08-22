#include "loom/graph/node.h"

namespace loom
{
    std::string landingPin(const NodeType& type, const std::string& carried, PinDirection side)
    {
        // A frame is drawn round the graph rather than joined to it, and an
        // entry point comes with its graph rather than from the menu.
        if (type.isFrame() || type.isEntryPoint()) return {};

        for (const PinSpec& pin : type.pins(type.minExtraPins()))
        {
            // A variable is chosen from a list, so no wire reaches that pin.
            if (pin.direction != side || pin.type == PinType::VariableName) continue;

            // A pin typed by a variable nobody has chosen yet carries something
            // unknown rather than nothing, so it is offered to any wire but a
            // flow. Whether the wire may really be drawn is settled once the
            // author makes the choice.
            if (!pin.typeFollows.empty())
            {
                if (carried != PinType::Flow) return pin.name;

                continue;
            }

            const bool joins = side == PinDirection::Input ? isCompatible(carried, pin.type)
                                                           : isCompatible(pin.type, carried);

            if (joins) return pin.name;
        }

        return {};
    }
}
