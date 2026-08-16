#ifndef LOOM_GRAPH_GRAPH_H
#define LOOM_GRAPH_GRAPH_H
#include <map>
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

    // Variable type ids. The scalars share their names with PinType; the two
    // containers have no pin of their own and travel as Any.
    namespace VariableType
    {
        inline constexpr const char* Bool   = "bool";
        inline constexpr const char* Int    = "int";
        inline constexpr const char* Float  = "float";
        inline constexpr const char* String = "string";
        inline constexpr const char* Color  = "color";
        inline constexpr const char* Choice = "choice";
        inline constexpr const char* List   = "list";
        inline constexpr const char* Group  = "group";
    }

    // One global variable as the author declared it. The type is an editor hint
    // and a run-time assertion; it never reaches a pin.
    struct VariableSpec
    {
        std::string              type = VariableType::String;
        Value                    value;
        std::vector<std::string> choices;   // the allowed values of a Choice
    };

    // The pin type a variable of this declared type travels as. The containers
    // have no pin of their own, so they travel as Any.
    std::string pinTypeOfVariable(const std::string& variableType);

    // A whole work: several graphs plus the one that starts.
    struct Project
    {
        Meta                                meta;
        std::string                         entry;
        std::map<std::string, VariableSpec> variables;
        std::vector<Graph>                  graphs;

        const Graph* findGraph(const std::string& name) const;

        // What a pin actually carries: one that follows a variable is whatever
        // that variable was declared to be, or Unset until one is chosen.
        std::string resolvedPinType(const NodeInstance& node, const PinSpec& pin) const;
    };
}

#endif //LOOM_GRAPH_GRAPH_H
