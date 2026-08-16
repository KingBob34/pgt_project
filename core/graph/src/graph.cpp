#include "loom/graph/graph.h"

#include "loom/value/inspect.h"

namespace loom
{
    std::string pinTypeOfVariable(const std::string& variableType)
    {
        if (variableType == VariableType::Bool)  return PinType::Bool;
        if (variableType == VariableType::Int)   return PinType::Int;
        if (variableType == VariableType::Float) return PinType::Float;
        if (variableType == VariableType::Color) return PinType::Color;

        // A choice is one of a list of strings; the containers have no pin.
        if (variableType == VariableType::String || variableType == VariableType::Choice)
        {
            return PinType::String;
        }

        return PinType::Any;
    }

    const NodeInstance* Graph::findNode(NodeId id) const
    {
        for (const NodeInstance& node : nodes)
        {
            if (node.id == id) return &node;
        }

        return nullptr;
    }

    const Connection* Graph::outgoing(NodeId from, const std::string& pin) const
    {
        for (const Connection& connection : connections)
        {
            if (connection.from == from && connection.fromPin == pin) return &connection;
        }

        return nullptr;
    }

    const Connection* Graph::incoming(NodeId to, const std::string& pin) const
    {
        for (const Connection& connection : connections)
        {
            if (connection.to == to && connection.toPin == pin) return &connection;
        }

        return nullptr;
    }

    const Graph* Project::findGraph(const std::string& name) const
    {
        for (const Graph& graph : graphs)
        {
            if (graph.name == name) return &graph;
        }

        return nullptr;
    }

    std::string Project::resolvedPinType(const NodeInstance& node, const PinSpec& pin) const
    {
        if (pin.typeFollows.empty()) return pin.type;

        const auto chosen = node.pinValues.find(pin.typeFollows);
        if (chosen == node.pinValues.end()) return PinType::Unset;

        const auto declared = variables.find(asString(chosen->second));
        if (declared == variables.end()) return PinType::Unset;

        return pinTypeOfVariable(declared->second.type);
    }
}
