#ifndef LOOM_RUNTIME_TESTS_STUB_NODES_H
#define LOOM_RUNTIME_TESTS_STUB_NODES_H
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "loom/graph/catalog.h"
#include "loom/value/inspect.h"

// A handful of node types invented purely for these tests. Nothing in
// core/runtime knows their names, which is the point: registering a new
// type is all it takes to extend the engine.
namespace stub
{
    inline loom::PinSpec in(std::string name, std::string type, loom::Value defaultValue = loom::Value())
    {
        return { std::move(name), "", loom::PinDirection::Input, std::move(type), std::move(defaultValue) };
    }

    inline loom::PinSpec out(std::string name, std::string type)
    {
        return { std::move(name), "", loom::PinDirection::Output, std::move(type), loom::Value() };
    }

    // Entry point: one flow output.
    class Start : public loom::NodeType
    {
    public:
        std::string name()         const override { return "start"; }
        std::string displayName()  const override { return "Start"; }
        std::string category()     const override { return "Test"; }
        bool        isEntryPoint() const override { return true; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { out("out", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext&) const override
        {
            return loom::FlowResult::continueOn("out");
        }
    };

    // Pushes its text pin to the host, then carries on.
    class Say : public loom::NodeType
    {
    public:
        std::string name()        const override { return "say"; }
        std::string displayName() const override { return "Say"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     in("text", loom::PinType::String, loom::Value("")),
                     out("out", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            context.host().showText(context.inputString("text"), loom::TextStyle());
            return loom::FlowResult::continueOn("out");
        }
    };

    // Writes a constant onto its output slot, so a later node can read it.
    class Produce : public loom::NodeType
    {
    public:
        std::string name()        const override { return "produce"; }
        std::string displayName() const override { return "Produce"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     in("amount", loom::PinType::Int, loom::Value(0)),
                     out("out", loom::PinType::Flow),
                     out("result", loom::PinType::Int) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            context.setOutput("result", context.inputInt("amount"));
            return loom::FlowResult::continueOn("out");
        }
    };

    // Stores whatever reaches its value pin under a fixed variable name.
    class Remember : public loom::NodeType
    {
    public:
        std::string name()        const override { return "remember"; }
        std::string displayName() const override { return "Remember"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     in("target", loom::PinType::String, loom::Value("kept")),
                     in("value", loom::PinType::Any),
                     out("out", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            context.writeVariable(context.inputString("target"), context.input("value"));
            return loom::FlowResult::continueOn("out");
        }
    };

    // Reads a variable and hands its text to the host, so a test can see it.
    class Recall : public loom::NodeType
    {
    public:
        std::string name()        const override { return "recall"; }
        std::string displayName() const override { return "Recall"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     in("source", loom::PinType::String, loom::Value("kept")),
                     out("out", loom::PinType::Flow),
                     out("missing", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            loom::Value found;

            if (!context.readVariable(context.inputString("source"), found))
            {
                return loom::FlowResult::continueOn("missing");
            }

            context.host().showText(loom::toText(found), loom::TextStyle());

            return loom::FlowResult::continueOn("out");
        }
    };

    // Offers two options and suspends until the player picks one.
    class Ask : public loom::NodeType
    {
    public:
        std::string name()        const override { return "ask"; }
        std::string displayName() const override { return "Ask"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     out("first", loom::PinType::Flow),
                     out("second", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            context.host().askChoice({ loom::Option{ "first" }, loom::Option{ "second" } },
                                     loom::TextStyle());
            return loom::FlowResult::choose({ "first", "second" });
        }
    };

    // Leaves this graph for another one and does not come back.
    class Leave : public loom::NodeType
    {
    public:
        std::string name()        const override { return "leave"; }
        std::string displayName() const override { return "Leave"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow),
                     in("scene", loom::PinType::String, loom::Value("")) };
        }

        loom::FlowResult execute(loom::ExecutionContext& context) const override
        {
            return loom::FlowResult::jump(context.inputString("scene"));
        }
    };

    // Ends the story.
    class End : public loom::NodeType
    {
    public:
        std::string name()        const override { return "end"; }
        std::string displayName() const override { return "End"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow) };
        }

        loom::FlowResult execute(loom::ExecutionContext&) const override
        {
            return loom::FlowResult::stop();
        }
    };

    inline loom::NodeCatalog makeCatalog()
    {
        loom::NodeCatalog catalog;
        catalog.add(std::make_unique<Start>());
        catalog.add(std::make_unique<Say>());
        catalog.add(std::make_unique<Produce>());
        catalog.add(std::make_unique<Remember>());
        catalog.add(std::make_unique<Recall>());
        catalog.add(std::make_unique<Ask>());
        catalog.add(std::make_unique<Leave>());
        catalog.add(std::make_unique<End>());
        return catalog;
    }

    inline loom::NodeInstance node(loom::NodeId id, std::string type)
    {
        loom::NodeInstance instance;
        instance.id = id;
        instance.type = std::move(type);
        return instance;
    }
}

#endif //LOOM_RUNTIME_TESTS_STUB_NODES_H
