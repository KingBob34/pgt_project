#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "loom/graph/validate.h"

namespace
{
    // Reads a global and hands its value on, the way Get Variable does.
    class ReaderNode : public loom::NodeType
    {
    public:
        std::string name()         const override { return "reader"; }
        std::string displayName()  const override { return "Reader"; }
        std::string category()     const override { return "Test"; }
        bool        isEntryPoint() const override { return true; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            loom::PinSpec named{ "name", "", loom::PinDirection::Input,
                                 loom::PinType::VariableName, loom::Value("") };

            loom::PinSpec value{ "value", "", loom::PinDirection::Output,
                                 loom::PinType::Unset, loom::Value(), false, "name" };

            return { named, value };
        }
    };

    class SinkNode : public loom::NodeType
    {
    public:
        explicit SinkNode(std::string type) : wants(std::move(type)) {}

        std::string name()        const override { return "sink"; }
        std::string displayName() const override { return "Sink"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { { "in", "", loom::PinDirection::Input, wants, loom::Value() } };
        }

    private:
        std::string wants;
    };

    loom::NodeCatalog makeCatalog(const std::string& sinkWants)
    {
        loom::NodeCatalog catalog;
        catalog.add(std::make_unique<ReaderNode>());
        catalog.add(std::make_unique<SinkNode>(sinkWants));

        return catalog;
    }

    // reader(1).value wired into sink(2).in, reading the named variable.
    loom::Project projectReading(const std::string& variable)
    {
        loom::NodeInstance reader;
        reader.id = 1;
        reader.type = "reader";
        reader.pinValues["name"] = variable;

        loom::NodeInstance sink;
        sink.id = 2;
        sink.type = "sink";

        loom::Graph graph;
        graph.name = "scene";
        graph.nodes = { reader, sink };
        graph.connections = { { 1, "value", 2, "in" } };

        loom::Project project;
        project.entry = "scene";
        project.graphs.push_back(graph);

        return project;
    }

    void declare(loom::Project& project, const std::string& name, const std::string& type)
    {
        loom::VariableSpec spec;
        spec.type = type;

        project.variables[name] = spec;
    }
}

TEST_CASE("a pin takes the type of the variable chosen for it", "[graph][following]")
{
    loom::Project project = projectReading("gold");
    declare(project, "gold", loom::VariableType::Int);

    const loom::PinSpec value{ "value", "", loom::PinDirection::Output,
                               loom::PinType::Unset, loom::Value(), false, "name" };

    REQUIRE(project.resolvedPinType(project.graphs[0].nodes[0], value) == loom::PinType::Int);
}

TEST_CASE("a pin whose variable is not declared has no type yet", "[graph][following]")
{
    loom::Project project = projectReading("gold");

    const loom::PinSpec value{ "value", "", loom::PinDirection::Output,
                               loom::PinType::Unset, loom::Value(), false, "name" };

    REQUIRE(project.resolvedPinType(project.graphs[0].nodes[0], value) == loom::PinType::Unset);
}

TEST_CASE("a declared type decides the pin it travels on", "[graph][following]")
{
    REQUIRE(loom::pinTypeOfVariable(loom::VariableType::Int) == loom::PinType::Int);
    REQUIRE(loom::pinTypeOfVariable(loom::VariableType::List) == loom::PinType::List);
    REQUIRE(loom::pinTypeOfVariable(loom::VariableType::Choice) == loom::PinType::String);

    // A group has no pin, and no node is ever offered one to begin with.
    REQUIRE(loom::pinTypeOfVariable(loom::VariableType::Group) == loom::PinType::Any);
}

TEST_CASE("a wire that matches the declared type passes", "[graph][following]")
{
    loom::Project project = projectReading("gold");
    declare(project, "gold", loom::VariableType::Int);

    const loom::NodeCatalog catalog = makeCatalog(loom::PinType::Int);
    loom::Diagnostics diagnostics;

    loom::validate(project, catalog, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
}

TEST_CASE("redeclaring the variable leaves the wire reported", "[graph][following]")
{
    loom::Project project = projectReading("gold");
    declare(project, "gold", loom::VariableType::String);

    const loom::NodeCatalog catalog = makeCatalog(loom::PinType::Int);
    loom::Diagnostics diagnostics;

    loom::validate(project, catalog, diagnostics);

    REQUIRE(diagnostics.hasErrors());
}

TEST_CASE("a wire is not judged while no variable is chosen", "[graph][following]")
{
    loom::Project project = projectReading("");

    const loom::NodeCatalog catalog = makeCatalog(loom::PinType::Int);
    loom::Diagnostics diagnostics;

    loom::validate(project, catalog, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
}
