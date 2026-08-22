#include "node_palette.h"

#include "loom/graph/pin.h"

#include <map>
#include <string>

namespace
{
    // Muted rather than bright: a canvas holds dozens of these at once, and
    // the wires and the text on them have to stay the things you read first.
    const std::map<std::string, QColor>& byCategory()
    {
        static const std::map<std::string, QColor> table = {
            { "Story",      QColor(0x3a, 0x5a, 0x7a) },
            { "Variables",  QColor(0x7a, 0x5a, 0x38) },
            { "Lists",      QColor(0x5f, 0x4a, 0x7a) },
            { "Values",     QColor(0x3a, 0x6b, 0x5a) },
            { "Maths",      QColor(0x6b, 0x63, 0x38) },
            { "Logic",      QColor(0x78, 0x3d, 0x7d) },
            { "Conversion", QColor(0x3a, 0x6b, 0x7a) },
            { "Utility",    QColor(0x4a, 0x4a, 0x4a) },
        };

        return table;
    }

    // Where the flow enters the graph, or runs out. Blueprint marks its events
    // the same way, and for the same reason: these are the ends of the thread.
    bool atTheEdgeOfFlow(const loom::NodeType& type)
    {
        if (type.isEntryPoint()) return true;

        bool takesFlow = false;

        for (const loom::PinSpec& pin : type.pins(type.minExtraPins()))
        {
            if (pin.type != loom::PinType::Flow) continue;

            if (pin.direction == loom::PinDirection::Output) return false;

            takesFlow = true;
        }

        return takesFlow;
    }
}

namespace
{
    // Type colours, kept in the same family Blueprint uses so that an author
    // who has seen a node editor before already knows most of them.
    const std::map<std::string, QColor>& byPinType()
    {
        static const std::map<std::string, QColor> table = {
            { loom::PinType::Flow,   QColor(0xe2, 0xe2, 0xe2) },
            { loom::PinType::Bool,   QColor(0xa8, 0x44, 0x3c) },
            { loom::PinType::Int,    QColor(0x3f, 0xae, 0x8f) },
            { loom::PinType::Float,  QColor(0x8c, 0xc2, 0x3c) },
            { loom::PinType::String, QColor(0xc7, 0x57, 0xb4) },
            { loom::PinType::Color,  QColor(0x6a, 0x78, 0xd0) },
            { loom::PinType::List,   QColor(0xc9, 0x96, 0x3c) },
            { loom::PinType::Any,    QColor(0x9a, 0xa2, 0xab) },
            { loom::PinType::Unset,  QColor(0x6a, 0x6a, 0x6a) },
        };

        return table;
    }
}

namespace palette
{
    QColor title(const loom::NodeType& type)
    {
        if (atTheEdgeOfFlow(type)) return QColor(0x8c, 0x3a, 0x3a);

        const auto found = byCategory().find(type.category());

        return found == byCategory().end() ? QColor(0x4a, 0x4a, 0x4a) : found->second;
    }

    QColor body()
    {
        return QColor(0x32, 0x32, 0x35);
    }

    QColor border(bool selected)
    {
        return selected ? QColor(0xd8, 0xbc, 0x6a) : QColor(0x1c, 0x1c, 0x1e);
    }

    QColor fault()
    {
        return QColor(0xd8, 0x4a, 0x4a);
    }

    QColor ready()
    {
        return QColor(0x5f, 0xbf, 0x66);
    }

    QColor caption()
    {
        return QColor(0xea, 0xea, 0xea);
    }

    QColor pin(const std::string& type)
    {
        const auto found = byPinType().find(type);

        return found == byPinType().end() ? byPinType().at(loom::PinType::Any) : found->second;
    }
}
