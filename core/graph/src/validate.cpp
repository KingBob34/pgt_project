#include "loom/graph/validate.h"

#include <map>
#include <set>

#include "loom/value/inspect.h"

namespace loom
{
    namespace
    {
        const PinSpec* findPin(const std::vector<PinSpec>& pins, const std::string& name)
        {
            for (const PinSpec& pin : pins)
            {
                if (pin.name == name) return &pin;
            }

            return nullptr;
        }

        std::string pinKey(NodeId node, const std::string& pin)
        {
            return std::to_string(node) + "/" + pin;
        }

        // A wire between two pins that were compatible when it was drawn, and
        // are not any more because a variable was rechosen or redeclared.
        void checkFollowingWires(const Project& project, const NodeCatalog& catalog,
                                 Diagnostics& out)
        {
            for (const Graph& graph : project.graphs)
            {
                for (const Connection& wire : graph.connections)
                {
                    const NodeInstance* from = graph.findNode(wire.from);
                    const NodeInstance* to   = graph.findNode(wire.to);

                    if (from == nullptr || to == nullptr) continue;

                    const NodeType* fromType = catalog.find(from->type);
                    const NodeType* toType   = catalog.find(to->type);

                    if (fromType == nullptr || toType == nullptr) continue;

                    const std::vector<PinSpec> fromPins = fromType->pins(from->extraPins);
                    const std::vector<PinSpec> toPins   = toType->pins(to->extraPins);

                    const PinSpec* source = findPin(fromPins, wire.fromPin);
                    const PinSpec* target = findPin(toPins, wire.toPin);

                    if (source == nullptr || target == nullptr) continue;
                    if (source->typeFollows.empty() && target->typeFollows.empty()) continue;

                    const std::string carried = project.resolvedPinType(*from, *source);
                    const std::string wanted  = project.resolvedPinType(*to, *target);

                    // No variable chosen yet means unknown, not wrong; the
                    // reference itself is reported elsewhere.
                    if (carried == PinType::Unset || wanted == PinType::Unset) continue;

                    if (isCompatible(carried, wanted)) continue;

                    out.error("this wire carries " + pinTypeLabel(carried) + " into " +
                              pinTypeLabel(wanted) + ", which it no longer fits",
                              graph.name, to->id, target->name);
                }
            }
        }

        // Every pin that names a global must name one the story declares. The
        // pin's type says which pins those are, so no node type is named here.
        void checkVariableReferences(const Project& project, const NodeCatalog& catalog,
                                     Diagnostics& out)
        {
            for (const Graph& graph : project.graphs)
            {
                for (const NodeInstance& node : graph.nodes)
                {
                    const NodeType* type = catalog.find(node.type);
                    if (type == nullptr) continue;

                    for (const PinSpec& pin : type->pins(node.extraPins))
                    {
                        if (pin.type != PinType::VariableName) continue;

                        const auto stored = node.pinValues.find(pin.name);

                        const std::string named = stored == node.pinValues.end()
                                                ? std::string()
                                                : asString(stored->second);

                        if (named.empty())
                        {
                            out.warning("no variable is chosen", graph.name, node.id, pin.name);
                            continue;
                        }

                        // A warning, so the story still opens and can be fixed.
                        if (project.variables.count(named) == 0)
                        {
                            out.warning("no variable called '" + named + "' is declared",
                                        graph.name, node.id, pin.name);
                        }
                    }
                }
            }
        }
    }

    void validate(const Graph& graph, const NodeCatalog& catalog, Diagnostics& out)
    {
        std::map<NodeId, std::vector<PinSpec>> pinsById;
        std::set<NodeId> ids;
        int entryPoints = 0;

        for (const NodeInstance& node : graph.nodes)
        {
            if (!ids.insert(node.id).second)
            {
                out.error("duplicate node id", graph.name, node.id);
                continue;
            }

            const NodeType* type = catalog.find(node.type);
            if (type == nullptr)
            {
                out.error("unknown node type '" + node.type + "'", graph.name, node.id);
                continue;
            }

            if (node.extraPins < type->minExtraPins() || node.extraPins > type->maxExtraPins())
            {
                out.error("extra pin count " + std::to_string(node.extraPins) + " is out of range",
                          graph.name, node.id);
                continue;
            }

            if (type->isEntryPoint()) ++entryPoints;

            std::vector<PinSpec> pins = type->pins(node.extraPins);

            for (const auto& stored : node.pinValues)
            {
                const PinSpec* pin = findPin(pins, stored.first);
                if (pin == nullptr)
                {
                    out.error("stored value for a pin this node does not have",
                              graph.name, node.id, stored.first);
                    continue;
                }

                if (isNull(stored.second)) continue;

                if (!canHold(pin->type, typeName(stored.second)))
                {
                    out.error("stored value is " + pinTypeLabel(typeName(stored.second)) +
                              ", pin expects " + pinTypeLabel(pin->type),
                              graph.name, node.id, pin->name);
                }
            }

            pinsById[node.id] = std::move(pins);
        }

        if (entryPoints == 0) out.error("the graph has no entry point", graph.name);
        if (entryPoints > 1) out.error("the graph has more than one entry point", graph.name);

        std::set<std::string> arrived;
        std::set<std::string> left;
        std::map<std::string, int> flowOutWires;
        std::map<std::string, int> dataInWires;

        for (const Connection& connection : graph.connections)
        {
            const auto fromPins = pinsById.find(connection.from);
            const auto toPins   = pinsById.find(connection.to);

            if (fromPins == pinsById.end() || toPins == pinsById.end())
            {
                out.error("connection references a node that does not exist", graph.name);
                continue;
            }

            const PinSpec* source = findPin(fromPins->second, connection.fromPin);
            if (source == nullptr)
            {
                out.error("no such pin on the source node", graph.name, connection.from, connection.fromPin);
                continue;
            }

            const PinSpec* target = findPin(toPins->second, connection.toPin);
            if (target == nullptr)
            {
                out.error("no such pin on the target node", graph.name, connection.to, connection.toPin);
                continue;
            }

            if (source->direction != PinDirection::Output)
            {
                out.error("a connection must start at an output pin",
                          graph.name, connection.from, connection.fromPin);
                continue;
            }

            if (target->direction != PinDirection::Input)
            {
                out.error("a connection must end at an input pin",
                          graph.name, connection.to, connection.toPin);
                continue;
            }

            // A pin that follows a variable is judged by the project, which is
            // the only place the declared variables are known.
            const bool follows = !source->typeFollows.empty() || !target->typeFollows.empty();

            if (!follows && !isCompatible(source->type, target->type))
            {
                out.error(pinTypeLabel(source->type) + " cannot connect to " + pinTypeLabel(target->type),
                          graph.name, connection.to, connection.toPin);
                continue;
            }

            if (source->type == PinType::Flow)
            {
                if (++flowOutWires[pinKey(connection.from, connection.fromPin)] > 1)
                {
                    out.error("a flow output may carry only one wire",
                              graph.name, connection.from, connection.fromPin);
                }
            }
            else if (++dataInWires[pinKey(connection.to, connection.toPin)] > 1)
            {
                out.error("a data input may take only one wire",
                          graph.name, connection.to, connection.toPin);
            }

            left.insert(pinKey(connection.from, connection.fromPin));
            arrived.insert(pinKey(connection.to, connection.toPin));
        }

        for (const NodeInstance& node : graph.nodes)
        {
            const auto pins = pinsById.find(node.id);
            if (pins == pinsById.end()) continue;

            const NodeType* type = catalog.find(node.type);

            for (const PinSpec& pin : pins->second)
            {
                if (pin.type != PinType::Flow) continue;

                if (pin.direction == PinDirection::Output)
                {
                    if (left.count(pinKey(node.id, pin.name)) == 0)
                    {
                        out.warning("this flow output goes nowhere", graph.name, node.id, pin.name);
                    }
                }
                else if (!type->isEntryPoint() && arrived.count(pinKey(node.id, pin.name)) == 0)
                {
                    out.warning("nothing leads here, so this node never runs",
                                graph.name, node.id, pin.name);
                }
            }
        }
    }

    void validate(const Project& project, const NodeCatalog& catalog, Diagnostics& out)
    {
        std::set<std::string> names;

        for (const Graph& graph : project.graphs)
        {
            if (!names.insert(graph.name).second)
            {
                out.error("two graphs share the name '" + graph.name + "'", graph.name);
            }

            validate(graph, catalog, out);
        }

        if (project.findGraph(project.entry) == nullptr)
        {
            out.error("the entry graph '" + project.entry + "' does not exist", project.entry);
        }

        checkVariableReferences(project, catalog, out);
        checkFollowingWires(project, catalog, out);
    }
}
