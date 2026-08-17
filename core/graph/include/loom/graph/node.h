#ifndef LOOM_GRAPH_NODE_H
#define LOOM_GRAPH_NODE_H
#include <map>
#include <string>
#include <vector>

#include "loom/graph/execution.h"
#include "loom/graph/pin.h"
#include "loom/value/value.h"

namespace loom
{
    using NodeId = int;

    struct Position
    {
        double x = 0.0;
        double y = 0.0;
    };

    // One node on the canvas.
    struct NodeInstance
    {
        NodeId      id = 0;
        std::string type;   // key into the NodeCatalog
        Position    position;
        int         extraPins = 0;   // variadic pin count
        std::map<std::string, Value> pinValues;   // in-place values, keyed by pin name
    };

    // One node type. Stateless; the catalog holds a single instance of each.
    class NodeType
    {
    public:
        virtual ~NodeType() = default;

        virtual std::string name()        const = 0;   // "branch", written to file
        virtual std::string displayName() const = 0;   // "Branch"
        virtual std::string category()    const = 0;   // "Story"


        // Exactly one node with this flag must exist in every graph.
        virtual bool isEntryPoint() const { return false; }

        virtual int minExtraPins() const { return 0; }
        virtual int maxExtraPins() const { return 0; }

        virtual std::vector<PinSpec> pins(int extraPins) const = 0;

        // Value nodes have no flow pins and are never reached.
        virtual FlowResult execute(ExecutionContext& context) const { return FlowResult::stop(); }
    };
}

#endif //LOOM_GRAPH_NODE_H
