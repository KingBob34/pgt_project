#include "loom/runtime/interpreter.h"

#include <tuple>
#include <utility>

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

        // What one node sees while it runs.
        class RuntimeContext : public ExecutionContext
        {
        public:
            RuntimeContext(const Graph& graph,
                           const NodeInstance& node,
                           const std::vector<PinSpec>& pins,
                           std::map<PinRef, Value>& outputs,
                           std::map<std::string, Value>& variables,
                           Host& host)
                : graph(graph), node(node), pins(pins),
                  outputs(outputs), variables(variables), hostRef(host) {}

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
                const auto found = variables.find(name);
                if (found == variables.end()) return false;

                out = found->second;
                return true;
            }

            void writeVariable(const std::string& name, Value value) override
            {
                variables[name] = std::move(value);
            }

            Host& host() override { return hostRef; }

        private:
            const Graph&                  graph;
            const NodeInstance&           node;
            const std::vector<PinSpec>&   pins;
            std::map<PinRef, Value>&      outputs;
            std::map<std::string, Value>& variables;
            Host&                         hostRef;
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

    void Interpreter::start()
    {
        callStack.clear();
        variables.clear();
        outputs.clear();
        pending = Pending();
        done = false;

        callStack.emplace_back();

        if (!enter(project.entry))
        {
            done = true;
            return;
        }

        run();
    }

    bool Interpreter::enter(const std::string& graphName)
    {
        const Graph* graph = project.findGraph(graphName);
        if (graph == nullptr) return false;

        for (const NodeInstance& node : graph->nodes)
        {
            const NodeType* type = catalog.find(node.type);
            if (type != nullptr && type->isEntryPoint())
            {
                callStack.back() = Frame{ graph->name, node.id, {} };
                return true;
            }
        }

        return false;
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
        while (!done && pending.kind == Pending::Kind::None)
        {
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
            RuntimeContext context(*graph, *node, pins, outputs, variables, host);

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
        RuntimeContext context(*graph, *node, pins, outputs, variables, host);

        // Running the node again only re-issues the prompt: it suspended last
        // time without changing anything, so it will do the same now.
        type->execute(context);
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
