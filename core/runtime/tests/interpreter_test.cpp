#include <catch2/catch_test_macros.hpp>

#include "loom/runtime/interpreter.h"

#include "stub_nodes.h"
#include "test_host.h"

namespace
{
    loom::Graph makeGraph(std::string name,
                          std::vector<loom::NodeInstance> nodes,
                          std::vector<loom::Connection> connections)
    {
        loom::Graph graph;
        graph.name = std::move(name);
        graph.nodes = std::move(nodes);
        graph.connections = std::move(connections);
        return graph;
    }

    loom::Project makeProject(std::vector<loom::Graph> graphs)
    {
        loom::Project project;
        project.entry = graphs.front().name;
        project.graphs = std::move(graphs);
        return project;
    }
}

TEST_CASE("the interpreter walks the flow wires to the end", "[runtime][interpreter]")
{
    loom::NodeInstance say = stub::node(2, "say");
    say.pinValues["text"] = "the gate is shut";

    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), say, stub::node(3, "end") },
        { { 1, "out", 2, "in" }, { 2, "out", 3, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
    REQUIRE(host.lines == std::vector<std::string>{ "the gate is shut" });
}

TEST_CASE("a value written by one node is read by a later one", "[runtime][interpreter]")
{
    loom::NodeInstance produce = stub::node(2, "produce");
    produce.pinValues["amount"] = 25;

    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), produce, stub::node(3, "remember"), stub::node(4, "end") },
        { { 1, "out", 2, "in" },
          { 2, "out", 3, "in" },
          { 2, "result", 3, "value" },
          { 3, "out", 4, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());

    const loom::SaveState state = interpreter.save();
    REQUIRE(state.variables.at("kept") == 25);
}

TEST_CASE("an unconnected input falls back to the value stored on the pin", "[runtime][interpreter]")
{
    // No wire feeds "value", so Remember must read the pin's own contents.
    loom::NodeInstance remember = stub::node(2, "remember");
    remember.pinValues["value"] = "from the pin";

    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), remember, stub::node(3, "end") },
        { { 1, "out", 2, "in" }, { 2, "out", 3, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.save().variables.at("kept") == "from the pin");
}

TEST_CASE("an unconnected input with nothing stored falls back to the declared default",
          "[runtime][interpreter]")
{
    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), stub::node(2, "produce"), stub::node(3, "remember"),
          stub::node(4, "end") },
        { { 1, "out", 2, "in" },
          { 2, "out", 3, "in" },
          { 2, "result", 3, "value" },
          { 3, "out", 4, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.save().variables.at("kept") == 0);
}

TEST_CASE("a choice suspends the interpreter until the player picks", "[runtime][interpreter]")
{
    loom::NodeInstance first = stub::node(3, "say");
    first.pinValues["text"] = "you bribe the guard";

    loom::NodeInstance second = stub::node(4, "say");
    second.pinValues["text"] = "you walk away";

    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), stub::node(2, "ask"), first, second, stub::node(5, "end") },
        { { 1, "out", 2, "in" },
          { 2, "first", 3, "in" },
          { 2, "second", 4, "in" },
          { 3, "out", 5, "in" },
          { 4, "out", 5, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.waiting());
    REQUIRE_FALSE(interpreter.finished());
    REQUIRE(host.offered == std::vector<std::string>{ "first", "second" });
    REQUIRE(host.lines.empty());

    SECTION("picking the first option follows the first pin")
    {
        interpreter.choose(0);

        REQUIRE(interpreter.finished());
        REQUIRE(host.lines == std::vector<std::string>{ "you bribe the guard" });
    }

    SECTION("picking the second option follows the second pin")
    {
        interpreter.choose(1);

        REQUIRE(interpreter.finished());
        REQUIRE(host.lines == std::vector<std::string>{ "you walk away" });
    }

    SECTION("an out of range choice is ignored")
    {
        interpreter.choose(7);

        REQUIRE(interpreter.waiting());
        REQUIRE(host.lines.empty());
    }
}

TEST_CASE("several flow wires may arrive at the same input", "[runtime][interpreter]")
{
    // Both branches of the choice lead to node 5; no merge node is needed.
    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), stub::node(2, "ask"), stub::node(3, "say"), stub::node(4, "say"),
          stub::node(5, "end") },
        { { 1, "out", 2, "in" },
          { 2, "first", 3, "in" },
          { 2, "second", 4, "in" },
          { 3, "out", 5, "in" },
          { 4, "out", 5, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();
    interpreter.choose(1);

    REQUIRE(interpreter.finished());
}

TEST_CASE("a jump continues in another graph", "[runtime][interpreter]")
{
    loom::NodeInstance leave = stub::node(2, "leave");
    leave.pinValues["scene"] = "village";

    loom::NodeInstance arrival = stub::node(2, "say");
    arrival.pinValues["text"] = "the village square";

    const loom::Project project = makeProject({
        makeGraph("gate",
            { stub::node(1, "start"), leave },
            { { 1, "out", 2, "in" } }),
        makeGraph("village",
            { stub::node(1, "start"), arrival, stub::node(3, "end") },
            { { 1, "out", 2, "in" }, { 2, "out", 3, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
    REQUIRE(host.lines == std::vector<std::string>{ "the village square" });
    REQUIRE(interpreter.save().callStack.back().graphName == "village");
}

TEST_CASE("a jump to a scene that is not there is reported", "[runtime][interpreter]")
{
    // The author mistyped the name of the scene they meant.
    loom::NodeInstance leave = stub::node(2, "leave");
    leave.pinValues["scene"] = "vilage";

    const loom::Project project = makeProject({
        makeGraph("gate",
            { stub::node(1, "start"), leave },
            { { 1, "out", 2, "in" } }),
        makeGraph("village",
            { stub::node(1, "start"), stub::node(2, "end") },
            { { 1, "out", 2, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());

    // Stopping is not enough: the name that led nowhere has to be said, or the
    // story simply ends and the author has nothing to go on.
    bool named = false;

    for (const TestHost::Command& command : host.commands)
    {
        if (command.name != "error") continue;

        const loom::Value* detail = loom::objectGet(command.args, "detail");

        if (detail != nullptr && loom::asString(*detail).find("vilage") != std::string::npos)
        {
            named = true;
        }
    }

    REQUIRE(named);
}

TEST_CASE("a flow output with no wire ends the story", "[runtime][interpreter]")
{
    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start") }, {}) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
}

TEST_CASE("a run can begin part way through a scene", "[runtime][interpreter]")
{
    loom::NodeInstance first = stub::node(2, "say");
    first.pinValues["text"] = "the gate is shut";

    loom::NodeInstance second = stub::node(3, "say");
    second.pinValues["text"] = "the guard looks up";

    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), first, second, stub::node(4, "end") },
        { { 1, "out", 2, "in" }, { 2, "out", 3, "in" }, { 3, "out", 4, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.startAt("gate", 3);

    REQUIRE(interpreter.finished());

    // The entry point and everything before node 3 were never run.
    REQUIRE(host.lines == std::vector<std::string>{ "the guard looks up" });
}

TEST_CASE("starting at a node that is not there is reported, not crashed",
          "[runtime][interpreter]")
{
    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start") }, {}) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.startAt("gate", 99);

    REQUIRE(interpreter.finished());
    REQUIRE(host.commands.size() == 1);
    REQUIRE(host.commands.front().name == "error");
}

TEST_CASE("a story that loops for ever is cut off rather than hanging",
          "[runtime][interpreter]")
{
    // Two nodes wired into a ring, with nothing in it that ever suspends.
    const loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start"), stub::node(2, "say"), stub::node(3, "say") },
        { { 1, "out", 2, "in" }, { 2, "out", 3, "in" }, { 3, "out", 2, "in" } }) });

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
    REQUIRE(host.commands.size() == 1);
    REQUIRE(host.commands.front().name == "error");
}

TEST_CASE("a missing entry graph stops rather than crashing", "[runtime][interpreter]")
{
    loom::Project project = makeProject({ makeGraph("gate",
        { stub::node(1, "start") }, {}) });
    project.entry = "nowhere";

    const loom::NodeCatalog catalog = stub::makeCatalog();
    TestHost host;

    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
}
