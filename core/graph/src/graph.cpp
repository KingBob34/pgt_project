#include "loom/graph/graph.h"

namespace loom
{
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
}
