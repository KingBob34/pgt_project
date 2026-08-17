#include <catch2/catch_test_macros.hpp>

#include "loom/runtime/interpreter.h"
#include "loom/value/inspect.h"

#include "stub_nodes.h"
#include "test_host.h"

namespace
{
    // start -> remember -> end, with the value to store sitting on the node.
    loom::Project projectWriting(loom::Value written)
    {
        loom::Graph graph;
        graph.name = "scene";
        graph.nodes = { stub::node(1, "start"), stub::node(2, "remember"), stub::node(3, "end") };
        graph.nodes[1].pinValues["value"] = std::move(written);
        graph.connections = { { 1, "out", 2, "in" }, { 2, "out", 3, "in" } };

        loom::Project project;
        project.entry = "scene";
        project.graphs.push_back(graph);

        return project;
    }

    void declare(loom::Project& project, const std::string& name,
                 const std::string& type, loom::Value start)
    {
        loom::VariableSpec spec;
        spec.type = type;
        spec.value = std::move(start);

        project.variables[name] = spec;
    }

    bool complained(const TestHost& host)
    {
        for (const TestHost::Command& command : host.commands)
        {
            if (command.name == "error") return true;
        }

        return false;
    }

    loom::SaveState play(const loom::Project& project, TestHost& host)
    {
        const loom::NodeCatalog catalog = stub::makeCatalog();

        loom::Interpreter interpreter(project, catalog, host);
        interpreter.start();

        return interpreter.save();
    }
}

TEST_CASE("a declared variable starts at the value the author gave it", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value(1));
    declare(project, "gold", loom::VariableType::Int, loom::Value(25));

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE(state.variables.count("gold") == 1);
    REQUIRE(loom::asInt(state.variables.at("gold")) == 25);
}

TEST_CASE("a variable of the declared type is stored without a word", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value(7));
    declare(project, "kept", loom::VariableType::Int, loom::Value(0));

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE_FALSE(complained(host));
    REQUIRE(loom::asInt(state.variables.at("kept")) == 7);
}

TEST_CASE("the wrong type is reported and stored all the same", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value("not a number"));
    declare(project, "kept", loom::VariableType::Int, loom::Value(0));

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE(complained(host));
    REQUIRE(loom::asString(state.variables.at("kept")) == "not a number");
}

TEST_CASE("an undeclared variable is judged against nothing", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value("anything at all"));

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE_FALSE(complained(host));
    REQUIRE(loom::asString(state.variables.at("kept")) == "anything at all");
}

TEST_CASE("a whole number satisfies a variable declared as a decimal", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value(3));
    declare(project, "kept", loom::VariableType::Float, loom::Value(0.0));

    TestHost host;
    play(project, host);

    REQUIRE_FALSE(complained(host));
}

TEST_CASE("a declared list holds whatever the author nested in it", "[runtime][variables]")
{
    loom::Value inventory = loom::Value::array();

    loom::Value sword = loom::Value::object();
    sword["name"] = "sword";
    sword["count"] = 1;

    inventory.push_back(sword);

    loom::Project project = projectWriting(loom::Value(1));
    declare(project, "inventory", loom::VariableType::List, inventory);

    TestHost host;
    const loom::SaveState state = play(project, host);

    const loom::Value& stored = state.variables.at("inventory");

    REQUIRE(loom::isList(stored));
    REQUIRE(loom::asString(*loom::objectGet(stored.front(), "name")) == "sword");
}

namespace
{
    // pet: { dog: { health: 100, name: "Rex" } }
    loom::Value pets()
    {
        loom::Value dog = loom::Value::object();
        dog["health"] = 100;
        dog["name"] = "Rex";

        loom::Value pet = loom::Value::object();
        pet["dog"] = dog;

        return pet;
    }

    long long healthOf(const loom::Value& pet)
    {
        return loom::asInt(*loom::objectGet(*loom::objectGet(pet, "dog"), "health"));
    }
}

TEST_CASE("a write reaches a field nested inside a group", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value(60));
    declare(project, "pet", loom::VariableType::Group, pets());

    project.graphs[0].nodes[1].pinValues["target"] = "pet.dog.health";

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE_FALSE(complained(host));
    REQUIRE(healthOf(state.variables.at("pet")) == 60);

    // Its neighbour is left where it was.
    REQUIRE(loom::asString(*loom::objectGet(*loom::objectGet(state.variables.at("pet"), "dog"),
                                            "name")) == "Rex");
}

TEST_CASE("a nested field is judged by the type it already holds", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value("badly"));
    declare(project, "pet", loom::VariableType::Group, pets());

    project.graphs[0].nodes[1].pinValues["target"] = "pet.dog.health";

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE(complained(host));

    // Reported, then written all the same.
    REQUIRE(loom::asString(*loom::objectGet(*loom::objectGet(state.variables.at("pet"), "dog"),
                                            "health")) == "badly");
}

TEST_CASE("a write to a field that was never declared changes nothing", "[runtime][variables]")
{
    loom::Project project = projectWriting(loom::Value(1));
    declare(project, "pet", loom::VariableType::Group, pets());

    project.graphs[0].nodes[1].pinValues["target"] = "pet.cat.health";

    TestHost host;
    const loom::SaveState state = play(project, host);

    REQUIRE(complained(host));
    REQUIRE(healthOf(state.variables.at("pet")) == 100);
    REQUIRE(loom::objectGet(state.variables.at("pet"), "cat") == nullptr);
}

TEST_CASE("a read follows the same path", "[runtime][variables]")
{
    loom::Graph graph;
    graph.name = "scene";
    graph.nodes = { stub::node(1, "start"), stub::node(2, "recall"), stub::node(3, "end") };
    graph.nodes[1].pinValues["source"] = "pet.dog.name";
    graph.connections = { { 1, "out", 2, "in" }, { 2, "out", 3, "in" } };

    loom::Project project;
    project.entry = "scene";
    project.graphs.push_back(graph);

    declare(project, "pet", loom::VariableType::Group, pets());

    TestHost host;
    play(project, host);

    REQUIRE(host.lines == std::vector<std::string>{ "Rex" });
}

TEST_CASE("a read of a field that is not there takes the second route",
          "[runtime][variables]")
{
    loom::Graph graph;
    graph.name = "scene";
    graph.nodes = { stub::node(1, "start"), stub::node(2, "recall"), stub::node(3, "end") };
    graph.nodes[1].pinValues["source"] = "pet.cat.name";
    graph.connections = { { 1, "out", 2, "in" }, { 2, "missing", 3, "in" } };

    loom::Project project;
    project.entry = "scene";
    project.graphs.push_back(graph);

    declare(project, "pet", loom::VariableType::Group, pets());

    TestHost host;
    play(project, host);

    REQUIRE(host.lines.empty());
}
