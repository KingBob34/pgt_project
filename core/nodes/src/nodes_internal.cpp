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
        return dataIn(std::move(name), std::move(label), PinType::Prose, prose::fromPlain(""));
    }

    void reportError(ExecutionContext& context, const NodeType& node, const std::string& detail)
    {
        Value details = makeObject();
        objectSet(details, "node", node.displayName());
        objectSet(details, "detail", detail);

        context.host().command("error", details);
    }

    bool readListVariable(ExecutionContext& context, const NodeType& node,
                          std::string& named, Value& held)
    {
        named = context.inputString("variable");

        if (!context.readVariable(named, held))
        {
            reportError(context, node, "there is no variable called '" + named + "'");
            return false;
        }

        if (!isList(held))
        {
            reportError(context, node, "'" + named + "' holds a " +
                                       pinTypeLabel(typeName(held)) + ", not a list");
            return false;
        }

        return true;
    }

    FlowResult orderedBy(ExecutionContext& context, const NodeType& node,
                         const std::function<bool(const Value&, const Value&)>& test)
    {
        const Value left = context.input("left");
        const Value right = context.input("right");

        if (!isNumber(left) || !isNumber(right))
        {
            reportError(context, node, "cannot order " + pinTypeLabel(typeName(left)) +
                                       " and " + pinTypeLabel(typeName(right)));

            context.setOutput("result", false);

            return FlowResult::stop();
        }

        context.setOutput("result", test(left, right));

        return FlowResult::stop();
    }
}
