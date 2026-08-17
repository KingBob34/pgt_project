#include "loom/graph/graph.h"

#include <algorithm>

#include "loom/value/inspect.h"
#include "loom/value/path.h"

namespace loom
{
    namespace
    {
        // A nested field carries no declaration of its own, so its type is
        // whatever its value already is.
        std::string typeOfValue(const Value& value)
        {
            if (isObject(value)) return VariableType::Group;
            if (isList(value))   return VariableType::List;
            if (isBool(value))   return VariableType::Bool;
            if (isInt(value))    return VariableType::Int;
            if (isFloat(value))  return VariableType::Float;

            return VariableType::String;
        }

        void gather(const std::string& prefix, const Value& value, std::vector<std::string>& out)
        {
            // A group is a way through, not a destination: nothing can build one,
            // so offering it would only let a node write rubbish over it.
            if (!isObject(value))
            {
                out.push_back(prefix);
                return;
            }

            for (const std::string& key : objectKeys(value))
            {
                if (const Value* field = objectGet(value, key))
                {
                    gather(prefix + "." + key, *field, out);
                }
            }
        }
    }

    std::string pinTypeOfVariable(const std::string& variableType)
    {
        if (variableType == VariableType::Bool)  return PinType::Bool;
        if (variableType == VariableType::Int)   return PinType::Int;
        if (variableType == VariableType::Float) return PinType::Float;
        if (variableType == VariableType::Color) return PinType::Color;
        if (variableType == VariableType::List)  return PinType::List;

        // A choice is one of a list of strings.
        if (variableType == VariableType::String || variableType == VariableType::Choice)
        {
            return PinType::String;
        }

        // A group has no pin of its own and is never offered to a node.
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

    std::vector<std::string> variablePaths(const std::map<std::string, VariableSpec>& variables)
    {
        std::vector<std::string> paths;

        for (const auto& entry : variables) gather(entry.first, entry.second.value, paths);

        std::sort(paths.begin(), paths.end());

        return paths;
    }

    std::string declaredTypeAt(const std::map<std::string, VariableSpec>& variables,
                               const std::string& path)
    {
        const std::vector<std::string> segments = splitPath(path);

        const auto declared = variables.find(segments.front());
        if (declared == variables.end()) return std::string();

        if (segments.size() == 1) return declared->second.type;

        const Value* field = descend(declared->second.value, segments, 1);

        return field == nullptr ? std::string() : typeOfValue(*field);
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

        const std::string declared = declaredTypeAt(variables, asString(chosen->second));

        return declared.empty() ? PinType::Unset : pinTypeOfVariable(declared);
    }
}
