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

        // Works out a value and pushes nothing along. Having no flow pins is
        // what this declaration looks like on the canvas, not what decides it:
        // a node that simply forgot its flow pins is a mistake, not a pure one.
        //
        // A pure node is run at the moment something reads one of its outputs,
        // once for each read, and its FlowResult is discarded.
        virtual bool isPure() const { return false; }

        // The author drags this node to whatever size suits what is on it.
        // The size is kept on hidden pins, so it travels with the node into
        // the file without the engine knowing what it is for.
        virtual bool isResizable() const { return isFrame(); }

        // A frame the author draws round part of the graph. It takes no wires
        // and the story never runs it; the editor gives it a size of its own
        // and puts it behind everything else.
        virtual bool isFrame() const { return false; }

        virtual int minExtraPins() const { return 0; }
        virtual int maxExtraPins() const { return 0; }

        virtual std::vector<PinSpec> pins(int extraPins) const = 0;

        // Value nodes have no flow pins and are never reached.
        virtual FlowResult execute(ExecutionContext& context) const { return FlowResult::stop(); }
    };

    // The pin a loose wire would land on, named, or empty when this type has
    // none. 'carried' is what the end already on the canvas holds and 'side'
    // is the side of this node the wire needs.
    //
    // The first pin that fits wins, so a node with several of a kind takes the
    // wire on its topmost one.
    std::string landingPin(const NodeType& type, const std::string& carried, PinDirection side);
}

#endif //LOOM_GRAPH_NODE_H
