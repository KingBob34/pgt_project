#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "loom/graph/validate.h"

namespace
{
    // A node type that exists only to be validated: pins in, nothing else.
    class FakeNode : public loom::NodeType
    {
    public:
        FakeNode(std::string typeName, std::vector<loom::PinSpec> pinSpecs, bool isEntry = false)
            : id(std::move(typeName)), specs(std::move(pinSpecs)), entry(isEntry) {}

        std::string name()         const override { return id; }
        std::string displayName()  const override { return id; }
        std::string category()     const override { return "Test"; }
        bool        isEntryPoint() const override { return entry; }

        std::vector<loom::PinSpec> pins(int) const override { return specs; }

    private:
        std::string                id;
        std::vector<loom::PinSpec> specs;
        bool                       entry = false;
    };

    loom::PinSpec in(std::string name, std::string type)
    {
        return { std::move(name), "", loom::PinDirection::Input, std::move(type), loom::Value() };
    }

    loom::PinSpec out(std::string name, std::string type)
    {
        return { std::move(name), "", loom::PinDirection::Output, std::move(type), loom::Value() };
    }

    loom::NodeCatalog makeCatalog()
    {
        loom::NodeCatalog catalog;

        catalog.add(std::make_unique<FakeNode>("start",
            std::vector<loom::PinSpec>{ out("out", loom::PinType::Flow) }, true));

        catalog.add(std::make_unique<FakeNode>("sink",
            std::vector<loom::PinSpec>{ in("in", loom::PinType::Flow) }));

        catalog.add(std::make_unique<FakeNode>("text",
            std::vector<loom::PinSpec>{ out("value", loom::PinType::String) }));

        catalog.add(std::make_unique<FakeNode>("consume", std::vector<loom::PinSpec>{
            in("in", loom::PinType::Flow),
            in("amount", loom::PinType::Int),
            out("out", loom::PinType::Flow) }));

        return catalog;
    }

    loom::NodeInstance node(loom::NodeId id, std::string type)
    {
        loom::NodeInstance instance;
        instance.id = id;
        instance.type = std::move(type);
        return instance;
    }

    loom::Graph makeGraph(std::vector<loom::NodeInstance> nodes,
                          std::vector<loom::Connection> connections)
    {
        loom::Graph graph;
        graph.name = "test";
        graph.nodes = std::move(nodes);
        graph.connections = std::move(connections);
        return graph;
    }

    bool mentions(const loom::Diagnostics& diagnostics, const std::string& fragment)
    {
        for (const loom::Diagnostic& entry : diagnostics.all())
        {
            if (entry.message.find(fragment) != std::string::npos) return true;
        }

        return false;
    }
}

TEST_CASE("a complete graph reports nothing", "[graph][validate]")
{
    const loom::NodeCatalog catalog = makeCatalog();
    const loom::Graph graph = makeGraph(
        { node(1, "start"), node(2, "sink") },
        { { 1, "out", 2, "in" } });

    loom::Diagnostics diagnostics;
    loom::validate(graph, catalog, diagnostics);

    REQUIRE(diagnostics.all().empty());
}

TEST_CASE("structural errors are reported", "[graph][validate]")
{
    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics diagnostics;

    SECTION("unknown node type")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "wat") }, {}), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "unknown node type 'wat'"));
    }

    SECTION("duplicate node id")
    {
        loom::validate(makeGraph({ node(1, "start"), node(1, "sink") }, {}), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "duplicate node id"));
    }

    SECTION("no entry point")
    {
        loom::validate(makeGraph({ node(1, "sink") }, {}), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "no entry point"));
    }

    SECTION("two entry points")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "start") }, {}), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "more than one entry point"));
    }

    REQUIRE(diagnostics.hasErrors());
}

TEST_CASE("connection errors name the offending pin", "[graph][validate]")
{
    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics diagnostics;

    SECTION("no such pin")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "sink") },
                                 { { 1, "nope", 2, "in" } }), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "no such pin on the source node"));
    }

    SECTION("incompatible types")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "consume"), node(3, "text") },
                                 { { 1, "out", 2, "in" }, { 3, "value", 2, "amount" } }),
                       catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "String cannot connect to Integer"));
    }

    SECTION("a connection into an output pin")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "consume") },
                                 { { 1, "out", 2, "out" } }), catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "must end at an input pin"));
    }

    SECTION("two wires on one flow output")
    {
        loom::validate(makeGraph({ node(1, "start"), node(2, "sink"), node(3, "consume") },
                                 { { 1, "out", 2, "in" }, { 1, "out", 3, "in" } }),
                       catalog, diagnostics);
        REQUIRE(mentions(diagnostics, "only one wire"));
    }

    REQUIRE(diagnostics.hasErrors());
}

TEST_CASE("a stored pin value must match the pin's type", "[graph][validate]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::NodeInstance consumer = node(2, "consume");
    consumer.pinValues["amount"] = "twelve";

    loom::Diagnostics diagnostics;
    loom::validate(makeGraph({ node(1, "start"), consumer }, { { 1, "out", 2, "in" } }),
                   catalog, diagnostics);

    REQUIRE(diagnostics.hasErrors());
    REQUIRE(mentions(diagnostics, "pin expects Integer"));
}

TEST_CASE("an unfinished graph produces warnings, not errors", "[graph][validate]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Diagnostics diagnostics;
    loom::validate(makeGraph({ node(1, "start"), node(2, "sink") }, {}), catalog, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(mentions(diagnostics, "goes nowhere"));
    REQUIRE(mentions(diagnostics, "never runs"));
}
