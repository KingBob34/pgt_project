#include "loom/runtime/interpreter.h"

#include <tuple>
#include <utility>

#include "loom/value/inspect.h"
#include "loom/value/path.h"

namespace loom
{
    namespace
    {
        // How many nodes one player action may set off. Far more than a scene
        // needs, and reached in an instant by a story that circles forever.
        constexpr long long kStepBudget = 100000;

        const PinSpec* findPin(const std::vector<PinSpec>& pins, const std::string& name)
        {
            for (const PinSpec& pin : pins)
            {
                if (pin.name == name) return &pin;
            }

            return nullptr;
        }

        void reportFault(Host& host, const std::string& detail)
        {
            Value details = Value::object();
            details["detail"] = detail;

            host.command("error", details);
        }

        // Whether a value is what the author declared the variable to hold. A
        // type this build does not know cannot be judged, so it passes.
        bool matchesType(const std::string& type, const Value& value)
        {
            if (type == VariableType::Bool)   return isBool(value);
            if (type == VariableType::Int)    return isInt(value);
            if (type == VariableType::Float)  return isNumber(value);
            if (type == VariableType::List)   return isList(value);
            if (type == VariableType::Group)  return isObject(value);

            // Colours and choices are both stored as text.
            if (type == VariableType::String || type == VariableType::Color ||
                type == VariableType::Choice)
            {
                return isString(value);
            }

            return true;
        }

        // What one node sees while it runs.
        class RuntimeContext : public ExecutionContext
        {
        public:
            RuntimeContext(const Graph& graph,
                           const NodeInstance& node,
                           const std::vector<PinSpec>& pins,
                           std::map<PinRef, Value>& outputs,
                           std::map<std::string, Value>& variables,
                           const std::map<std::string, VariableSpec>& declared,
                           Host& host)
                : graph(graph), node(node), pins(pins),
                  outputs(outputs), variables(variables), declared(declared), hostRef(host) {}

            Value input(const std::string& pin) const override
            {
                if (const Connection* wire = graph.incoming(node.id, pin))
                {
                    const auto slot = outputs.find(PinRef{ graph.name, wire->from, wire->fromPin });
                    if (slot != outputs.end()) return slot->second;

                    // Nothing computed it: the source is a value node, whose
                    // constant sits on the node itself and is never executed.
                    if (const NodeInstance* source = graph.findNode(wire->from))
                    {
                        const auto constant = source->pinValues.find(wire->fromPin);
                        if (constant != source->pinValues.end()) return constant->second;
                    }
                }

                const auto stored = node.pinValues.find(pin);
                if (stored != node.pinValues.end()) return stored->second;

                if (const PinSpec* spec = findPin(pins, pin)) return spec->defaultValue;

                return Value();
            }

            void setOutput(const std::string& pin, Value value) override
            {
                outputs[PinRef{ graph.name, node.id, pin }] = std::move(value);
            }

            bool readVariable(const std::string& name, Value& out) const override
            {
                const std::vector<std::string> segments = splitPath(name);

                const auto found = variables.find(segments.front());
                if (found == variables.end()) return false;

                if (segments.size() == 1)
                {
                    out = found->second;
                    return true;
                }

                const Value* field = descend(found->second, segments, 1);
                if (field == nullptr) return false;

                out = *field;
                return true;
            }

            void writeVariable(const std::string& name, Value value) override
            {
                const std::string wanted = declaredTypeAt(declared, name);

                // Said out loud, then written anyway: the author's data is never
                // second-guessed, only reported on.
                if (!wanted.empty() && !matchesType(wanted, value))
                {
                    reportFault(hostRef, "the variable '" + name + "' is declared as " + wanted +
                                         " but node " + std::to_string(node.id) + " in scene '" +
                                         graph.name + "' wrote " + typeName(value) + " into it");
                }

                const std::vector<std::string> segments = splitPath(name);

                const auto found = variables.find(segments.front());

                if (found == variables.end())
                {
                    // A path can only reach into something that is already there.
                    if (segments.size() > 1)
                    {
                        reportFault(hostRef, "there is no variable called '" + segments.front() +
                                             "' to write '" + name + "' into");
                        return;
                    }

                    variables[name] = std::move(value);
                    return;
                }

                if (segments.size() == 1)
                {
                    found->second = std::move(value);
                    return;
                }

                if (!assign(found->second, segments, std::move(value), 1))
                {
                    reportFault(hostRef, "the variable '" + segments.front() +
                                         "' has no field at '" + name + "'");
                }
            }

            Host& host() override { return hostRef; }

        private:
            const Graph&                  graph;
            const NodeInstance&           node;
            const std::vector<PinSpec>&   pins;
            std::map<PinRef, Value>&      outputs;
            std::map<std::string, Value>& variables;

            const std::map<std::string, VariableSpec>& declared;

            Host& hostRef;
        };
    }

    bool PinRef::operator<(const PinRef& other) const
    {
        return std::tie(graphName, node, pin) < std::tie(other.graphName, other.node, other.pin);
    }


    bool PinRef::operator==(const PinRef& other) const
    {
        return std::tie(graphName, node, pin) == std::tie(other.graphName, other.node, other.pin);
    }

    Interpreter::Interpreter(const Project& project, const NodeCatalog& catalog, Host& host)
        : project(project), catalog(catalog), host(host)
    {
    }

    void Interpreter::reset()
    {
        callStack.clear();
        variables.clear();
        outputs.clear();
        pending = Pending();
        done = false;

        for (const auto& entry : project.variables) variables[entry.first] = entry.second.value;
    }

    void Interpreter::start()
    {
        reset();

        callStack.emplace_back();

        if (!enter(project.entry))
        {
            done = true;
            return;
        }

        run();
    }

    void Interpreter::startAt(const std::string& graphName, NodeId nodeId)
    {
        reset();

        const Graph* graph = project.findGraph(graphName);

        if (graph == nullptr || graph->findNode(nodeId) == nullptr)
        {
            report("there is nothing to start from at node " + std::to_string(nodeId) +
                   " in scene '" + graphName + "'");

            done = true;
            return;
        }

        callStack.push_back(Frame{ graphName, nodeId, {} });

        run();
    }

    bool Interpreter::enter(const std::string& graphName)
    {
        const Graph* graph = project.findGraph(graphName);

        if (graph == nullptr)
        {
            report("there is no scene called '" + graphName + "'");
            return false;
        }

        for (const NodeInstance& node : graph->nodes)
        {
            const NodeType* type = catalog.find(node.type);
            if (type != nullptr && type->isEntryPoint())
            {
                callStack.back() = Frame{ graph->name, node.id, {} };
                return true;
            }
        }

        report("the scene '" + graphName + "' has no entry point");

        return false;
    }

    void Interpreter::report(const std::string& detail) const
    {
        reportFault(host, detail);
    }

    bool Interpreter::advance(const std::string& pin)
    {
        Frame& frame = callStack.back();

        const Graph* graph = project.findGraph(frame.graphName);
        if (graph == nullptr) return false;

        const Connection* wire = graph->outgoing(frame.nodeId, pin);
        if (wire == nullptr) return false;

        frame.nodeId = wire->to;
        return true;
    }

    void Interpreter::run()
    {
        long long steps = 0;

        while (!done && pending.kind == Pending::Kind::None)
        {
            // A graph may loop, so a story can be written that never stops.
            // Without this the front end freezes with it.
            if (++steps > kStepBudget)
            {
                const Frame& stuck = callStack.back();

                report("the story ran " + std::to_string(kStepBudget) + " nodes without ever " +
                       "stopping, and was cut off at node " + std::to_string(stuck.nodeId) +
                       " in scene '" + stuck.graphName + "'");

                done = true;
                return;
            }

            const Frame& frame = callStack.back();

            const Graph* graph = project.findGraph(frame.graphName);
            const NodeInstance* node = graph != nullptr ? graph->findNode(frame.nodeId) : nullptr;
            const NodeType* type = node != nullptr ? catalog.find(node->type) : nullptr;

            if (type == nullptr)
            {
                done = true;
                return;
            }

            const std::vector<PinSpec> pins = type->pins(node->extraPins);
            RuntimeContext context(*graph, *node, pins, outputs, variables, project.variables, host);

            const FlowResult result = type->execute(context);

            switch (result.kind)
            {
                case FlowResult::Kind::Continue:
                    done = !advance(result.pin);
                    break;

                case FlowResult::Kind::Wait:
                    pending.kind = Pending::Kind::Click;
                    pending.pin = result.pin;
                    break;

                case FlowResult::Kind::Choose:
                    pending.kind = Pending::Kind::Choice;
                    pending.optionPins = result.optionPins;
                    break;

                case FlowResult::Kind::Jump:
                    done = !enter(result.targetGraph);
                    break;

                case FlowResult::Kind::Stop:
                    done = true;
                    break;
            }
        }
    }

    void Interpreter::resume()
    {
        if (pending.kind != Pending::Kind::Click) return;

        const std::string pin = pending.pin;
        pending = Pending();

        done = !advance(pin);
        run();
    }

    void Interpreter::choose(int index)
    {
        if (pending.kind != Pending::Kind::Choice) return;
        if (index < 0 || index >= static_cast<int>(pending.optionPins.size())) return;

        const std::string pin = pending.optionPins[index];
        pending = Pending();

        done = !advance(pin);
        run();
    }

    void Interpreter::replay()
    {
        if (pending.kind != Pending::Kind::Choice) return;

        const Frame& frame = callStack.back();

        const Graph* graph = project.findGraph(frame.graphName);
        const NodeInstance* node = graph != nullptr ? graph->findNode(frame.nodeId) : nullptr;
        const NodeType* type = node != nullptr ? catalog.find(node->type) : nullptr;

        if (type == nullptr) return;

        const std::vector<PinSpec> pins = type->pins(node->extraPins);
        RuntimeContext context(*graph, *node, pins, outputs, variables, project.variables, host);

        // Running the node again only re-issues the prompt: it suspended last
        // time without changing anything, so it will do the same now.
        type->execute(context);
    }

    std::string Interpreter::currentGraph() const
    {
        return callStack.empty() ? std::string() : callStack.back().graphName;
    }

    NodeId Interpreter::currentNode() const
    {
        return callStack.empty() ? 0 : callStack.back().nodeId;
    }

    bool Interpreter::finished() const
    {
        return done && pending.kind == Pending::Kind::None;
    }

    bool Interpreter::waiting() const
    {
        return pending.kind != Pending::Kind::None;
    }


    SaveState Interpreter::save() const
    {
        SaveState state;
        state.callStack = callStack;
        state.variables = variables;
        state.outputs = outputs;
        state.pending = pending;
        state.done = done;
        return state;
    }

    void Interpreter::restore(const SaveState& state)
    {
        callStack = state.callStack;
        variables = state.variables;
        outputs = state.outputs;
        pending = state.pending;
        done = state.done;
    }
}
