#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    // The one node type these tests need: an entry point with nothing on it.
    class OnlyNode : public loom::NodeType
    {
    public:
        std::string name()         const override { return "start"; }
        std::string displayName()  const override { return "Start"; }
        std::string category()     const override { return "Test"; }
        bool        isEntryPoint() const override { return true; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { { "out", "", loom::PinDirection::Output, loom::PinType::Flow, loom::Value() } };
        }
    };

    loom::NodeCatalog makeCatalog()
    {
        loom::NodeCatalog catalog;
        catalog.add(std::make_unique<OnlyNode>());

        return catalog;
    }

    loom::Project oneSceneProject()
    {
        loom::Graph graph;
        graph.name = "scene";

        loom::NodeInstance start;
        start.id = 1;
        start.type = "start";

        graph.nodes.push_back(start);

        loom::Project project;
        project.entry = "scene";
        project.graphs.push_back(graph);

        return project;
    }

    bool readsFrom(const std::string& text, const loom::NodeCatalog& catalog,
                   loom::Project& out, loom::Diagnostics& diagnostics)
    {
        loom::Value parsed;
        std::string error;

        REQUIRE(loom::parseJson(text, parsed, error));

        return loom::readProject(parsed, catalog, out, diagnostics);
    }
}

TEST_CASE("declared variables survive being written and read again", "[serialization][variables]")
{
    loom::Project project = oneSceneProject();

    loom::VariableSpec gold;
    gold.type = loom::VariableType::Int;
    gold.value = loom::Value(25);

    loom::VariableSpec mood;
    mood.type = loom::VariableType::Choice;
    mood.value = loom::Value("calm");
    mood.choices = { "calm", "angry" };

    project.variables["gold"] = gold;
    project.variables["mood"] = mood;

    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics     diagnostics;
    loom::Project         reloaded;

    REQUIRE(readsFrom(loom::writeJson(loom::writeProject(project)), catalog,
                      reloaded, diagnostics));

    REQUIRE(reloaded.variables.size() == 2);
    REQUIRE(reloaded.variables.at("gold").type == loom::VariableType::Int);
    REQUIRE(loom::asInt(reloaded.variables.at("gold").value) == 25);
    REQUIRE(reloaded.variables.at("mood").choices.size() == 2);
    REQUIRE(reloaded.variables.at("mood").choices.front() == "calm");
}

TEST_CASE("a nested initial value keeps its shape through the file", "[serialization][variables]")
{
    loom::Project project = oneSceneProject();

    loom::Value sword = loom::Value::object();
    sword["name"] = "sword";
    sword["count"] = 1;

    loom::Value inventory = loom::Value::array();
    inventory.push_back(sword);

    loom::VariableSpec bag;
    bag.type = loom::VariableType::List;
    bag.value = inventory;

    project.variables["inventory"] = bag;

    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics     diagnostics;
    loom::Project         reloaded;

    REQUIRE(readsFrom(loom::writeJson(loom::writeProject(project)), catalog,
                      reloaded, diagnostics));

    const loom::Value& stored = reloaded.variables.at("inventory").value;

    REQUIRE(loom::isList(stored));
    REQUIRE(loom::asInt(*loom::objectGet(stored.front(), "count")) == 1);
}

TEST_CASE("a story that declares nothing still opens", "[serialization][variables]")
{
    const std::string text =
        "{ \"schemaVersion\": " + std::to_string(loom::kSchemaVersion) +
        ", \"entry\": \"scene\", \"graphs\": [ { \"name\": \"scene\", \"nodes\": "
        "[ { \"id\": 1, \"type\": \"start\" } ] } ] }";

    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics     diagnostics;
    loom::Project         opened;

    REQUIRE(readsFrom(text, catalog, opened, diagnostics));

    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(opened.variables.empty());
    REQUIRE(opened.graphs.size() == 1);
}

TEST_CASE("a variable type this build does not know is kept and reported",
          "[serialization][variables]")
{
    const std::string text =
        "{ \"schemaVersion\": " + std::to_string(loom::kSchemaVersion) +
        ", \"entry\": \"scene\", \"variables\": { \"pet\": { \"type\": \"creature\", "
        "\"value\": \"cat\" } }, \"graphs\": [ { \"name\": \"scene\", \"nodes\": "
        "[ { \"id\": 1, \"type\": \"start\" } ] } ] }";

    const loom::NodeCatalog catalog = makeCatalog();
    loom::Diagnostics     diagnostics;
    loom::Project         opened;

    REQUIRE(readsFrom(text, catalog, opened, diagnostics));

    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(opened.variables.at("pet").type == "creature");
    REQUIRE(loom::asString(opened.variables.at("pet").value) == "cat");
}
