#include <catch2/catch_test_macros.hpp>

#include "loom/runtime/save.h"

#include "loom/value/parse.h"
#include "stub_nodes.h"
#include "test_host.h"

namespace
{
    loom::Project makeStory()
    {
        loom::NodeInstance produce = stub::node(2, "produce");
        produce.pinValues["amount"] = 25;

        loom::NodeInstance bribed = stub::node(5, "say");
        bribed.pinValues["text"] = "you bribe the guard";

        loom::NodeInstance walked = stub::node(6, "say");
        walked.pinValues["text"] = "you walk away";

        loom::Graph graph;
        graph.name = "gate";
        graph.nodes = { stub::node(1, "start"), produce, stub::node(3, "remember"),
                        stub::node(4, "ask"), bribed, walked, stub::node(7, "end") };
        graph.connections = { { 1, "out", 2, "in" },
                              { 2, "out", 3, "in" },
                              { 2, "result", 3, "value" },
                              { 3, "out", 4, "in" },
                              { 4, "first", 5, "in" },
                              { 4, "second", 6, "in" },
                              { 5, "out", 7, "in" },
                              { 6, "out", 7, "in" } };

        loom::Project project;
        project.entry = "gate";
        project.graphs = { graph };
        return project;
    }
}

TEST_CASE("a save taken mid-story restores to the same place", "[runtime][save]")
{
    const loom::Project project = makeStory();
    const loom::NodeCatalog catalog = stub::makeCatalog();

    TestHost first;
    loom::Interpreter original(project, catalog, first);
    original.start();

    REQUIRE(original.waiting());

    const loom::SaveState saved = original.save();

    // A fresh interpreter, as if the program had been closed and reopened.
    TestHost second;
    loom::Interpreter reloaded(project, catalog, second);
    reloaded.restore(saved);

    REQUIRE(reloaded.waiting());

    reloaded.choose(0);
    original.choose(0);

    REQUIRE(reloaded.finished());
    REQUIRE(reloaded.save().variables == original.save().variables);
    REQUIRE(second.lines == first.lines);
}

TEST_CASE("a save survives a trip through JSON", "[runtime][save]")
{
    const loom::Project project = makeStory();
    const loom::NodeCatalog catalog = stub::makeCatalog();

    TestHost host;
    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    const loom::SaveState saved = interpreter.save();

    std::string error;
    loom::Value reparsed;
    REQUIRE(loom::parseJson(loom::writeJson(loom::writeSave(saved)), reparsed, error));

    loom::SaveState restored;
    REQUIRE(loom::readSave(reparsed, restored, error));

    REQUIRE(restored.callStack.size() == saved.callStack.size());
    REQUIRE(restored.callStack.back().graphName == saved.callStack.back().graphName);
    REQUIRE(restored.callStack.back().nodeId == saved.callStack.back().nodeId);
    REQUIRE(restored.variables == saved.variables);
    REQUIRE(restored.outputs == saved.outputs);
    REQUIRE(restored.pending.kind == saved.pending.kind);
    REQUIRE(restored.pending.optionPins == saved.pending.optionPins);
    REQUIRE(restored.done == saved.done);
}

TEST_CASE("readSave refuses a file it does not recognise", "[runtime][save]")
{
    loom::SaveState state;
    std::string error;

    SECTION("no version at all")
    {
        loom::Value document = loom::Value::object();
        REQUIRE_FALSE(loom::readSave(document, state, error));
        REQUIRE(error.find("version") != std::string::npos);
    }

    SECTION("a version from the future")
    {
        loom::Value document = loom::Value::object();
        document["saveVersion"] = loom::kSaveVersion + 1;

        REQUIRE_FALSE(loom::readSave(document, state, error));
        REQUIRE(error.find("version") != std::string::npos);
    }

    SECTION("an empty call stack")
    {
        loom::Value document = loom::Value::object();
        document["saveVersion"] = loom::kSaveVersion;
        document["callStack"] = loom::Value::array();

        REQUIRE_FALSE(loom::readSave(document, state, error));
        REQUIRE(error.find("call stack") != std::string::npos);
    }
}
