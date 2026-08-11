#ifndef LOOM_GRAPH_GRAPH_H
#define LOOM_GRAPH_GRAPH_H
#include <string>
#include <vector>

#include "loom/graph/node.h"

namespace loom
{
    // A wire. Both ends name their pin; port indices are never stored.
    struct Connection
    {
        NodeId      from = 0;
        std::string fromPin;
        NodeId      to = 0;
        std::string toPin;
    };

    struct Meta
    {
        std::string title;
        std::string author;
    };

    // One scene: the nodes of one canvas and the wires between them.
    struct Graph
    {
        std::string               name;
        Meta                      meta;
        std::vector<NodeInstance> nodes;
        std::vector<Connection>   connections;

        const NodeInstance* findNode(NodeId id) const;

        // A flow output carries at most one wire, so one result is enough.
        const Connection* outgoing(NodeId from, const std::string& pin) const;

        // A data input carries at most one wire, so one result is enough.
        const Connection* incoming(NodeId to, const std::string& pin) const;
    };

    // A whole work: several graphs plus the one that starts.
    struct Project
    {
        Meta               meta;
        std::string        entry;
        std::vector<Graph> graphs;

        const Graph* findGraph(const std::string& name) const;
    };
}

#endif //LOOM_GRAPH_GRAPH_H
