#include <catch2/catch_test_macros.hpp>

#include <string>

#include "loom/serialization/graph_io.h"

#include "loom/value/inspect.h"

namespace
{
    // Node types invented for these tests: the serialization layer is not
    // allowed to know any real ones, and this is where that gets proven.
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

        catalog.add(std::make_unique<FakeNode>("sink", std::vector<loom::PinSpec>{
            in("in", loom::PinType::Flow),
            in("count", loom::PinType::Int),
            in("ratio", loom::PinType::Float),
            in("label", loom::PinType::String) }));

        return catalog;
    }

    loom::Graph makeGraph()
    {
        loom::NodeInstance start;
        start.id = 1;
        start.type = "start";
        start.position = { 0.0, 0.0 };

        loom::NodeInstance sink;
        sink.id = 2;
        sink.type = "sink";
        sink.position = { 440.0, -160.5 };
        sink.pinValues["count"] = 25;
        sink.pinValues["ratio"] = 2.5;
        sink.pinValues["label"] = "the gate is shut";

        loom::Graph graph;
        graph.name = "gate";
        graph.meta.title = "Gate Test";
        graph.meta.author = "KingBob";
        graph.nodes = { start, sink };
        graph.connections = { { 1, "out", 2, "in" } };

        return graph;
    }
}

TEST_CASE("a graph survives being written and read back", "[serialization][round trip]")
{
    const loom::NodeCatalog catalog = makeCatalog();
    const loom::Graph original = makeGraph();

    loom::Graph reloaded;
    loom::Diagnostics diagnostics;
    REQUIRE(loom::readGraph(loom::writeGraph(original), catalog, reloaded, diagnostics));

    REQUIRE_FALSE(diagnostics.hasErrors());

    REQUIRE(reloaded.name == original.name);
    REQUIRE(reloaded.meta.title == original.meta.title);
    REQUIRE(reloaded.meta.author == original.meta.author);
    REQUIRE(reloaded.nodes.size() == original.nodes.size());
    REQUIRE(reloaded.connections.size() == original.connections.size());

    const loom::NodeInstance* sink = reloaded.findNode(2);
    REQUIRE(sink != nullptr);
    REQUIRE(sink->type == "sink");
    REQUIRE(sink->position.x == 440.0);
    REQUIRE(sink->position.y == -160.5);

    // Int and float are separate pin types, so the distinction has to survive
    // the trip through text or every strict reader downstream breaks.
    REQUIRE(loom::isInt(sink->pinValues.at("count")));
    REQUIRE(loom::isFloat(sink->pinValues.at("ratio")));
    REQUIRE(loom::asInt(sink->pinValues.at("count")) == 25);
    REQUIRE(loom::asFloat(sink->pinValues.at("ratio")) == 2.5);
    REQUIRE(loom::asString(sink->pinValues.at("label")) == "the gate is shut");

    const loom::Connection& wire = reloaded.connections.front();
    REQUIRE(wire.from == 1);
    REQUIRE(wire.fromPin == "out");
    REQUIRE(wire.to == 2);
    REQUIRE(wire.toPin == "in");
}

TEST_CASE("writing twice gives the same document", "[serialization][round trip]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Graph reloaded;
    loom::Diagnostics diagnostics;
    const loom::Value once = loom::writeGraph(makeGraph());
    REQUIRE(loom::readGraph(once, catalog, reloaded, diagnostics));

    // A save that changes the file without the author changing anything makes
    // every diff useless, so the second pass has to land on the same bytes.
    REQUIRE(loom::writeGraph(reloaded) == once);
}

TEST_CASE("a project survives being written and read back", "[serialization][round trip]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Graph village = makeGraph();
    village.name = "village";

    loom::Project original;
    original.meta.title = "Gate Test";
    original.entry = "gate";
    original.graphs = { makeGraph(), village };

    loom::Project reloaded;
    loom::Diagnostics diagnostics;
    REQUIRE(loom::readProject(loom::writeProject(original), catalog, reloaded, diagnostics));

    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(reloaded.entry == "gate");
    REQUIRE(reloaded.graphs.size() == 2);
    REQUIRE(reloaded.findGraph("village") != nullptr);
}

TEST_CASE("a document from an unknown format version is refused", "[serialization][version]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Graph reloaded;
    loom::Diagnostics diagnostics;

    SECTION("no version at all")
    {
        loom::Value document = loom::writeGraph(makeGraph());
        document.erase("schemaVersion");

        REQUIRE_FALSE(loom::readGraph(document, catalog, reloaded, diagnostics));
    }

    SECTION("a version from the future")
    {
        loom::Value document = loom::writeGraph(makeGraph());
        document["schemaVersion"] = loom::kSchemaVersion + 1;

        REQUIRE_FALSE(loom::readGraph(document, catalog, reloaded, diagnostics));
    }

    REQUIRE(diagnostics.hasErrors());
}

TEST_CASE("one unreadable record does not cost the whole file", "[serialization][reader]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Value document = loom::writeGraph(makeGraph());

    // A third node with no type at all, as an outside tool might leave it.
    loom::Value broken = loom::Value::object();
    broken["id"] = 3;
    broken["position"] = loom::Value::object();
    document["nodes"].push_back(broken);

    loom::Graph reloaded;
    loom::Diagnostics diagnostics;

    REQUIRE(loom::readGraph(document, catalog, reloaded, diagnostics));
    REQUIRE(diagnostics.hasErrors());

    // The two good nodes are still there, and the fault names the bad one.
    REQUIRE(reloaded.nodes.size() == 2);
    REQUIRE(reloaded.findNode(1) != nullptr);
    REQUIRE(reloaded.findNode(2) != nullptr);
}

TEST_CASE("reading a graph validates it in the same call", "[serialization][reader]")
{
    const loom::NodeCatalog catalog = makeCatalog();

    loom::Value document = loom::writeGraph(makeGraph());
    document["connections"][0]["toPin"] = "nosuchpin";

    loom::Graph reloaded;
    loom::Diagnostics diagnostics;

    REQUIRE(loom::readGraph(document, catalog, reloaded, diagnostics));

    // Nothing asked for validation: a graph that has been read has been checked.
    REQUIRE(diagnostics.hasErrors());

    bool named = false;
    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        if (entry.pin == "nosuchpin") named = true;
    }
    REQUIRE(named);
}
