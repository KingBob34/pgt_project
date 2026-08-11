#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "loom/nodes/builtin.h"
#include "loom/runtime/interpreter.h"
#include "loom/runtime/save.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

// The first test that puts the whole engine together: a story written as JSON
// is read, validated and played through to each of its endings. Nothing here
// touches a window or a terminal, which is the point.
namespace
{
    const char* kStory = R"({
  "schemaVersion": 2,
  "meta": { "title": "Gate Test", "author": "KingBob" },
  "entry": "gate",
  "graphs": [
    {
      "name": "gate",
      "meta": { "title": "Gate Test", "author": "KingBob" },
      "nodes": [
        { "id": 1,  "type": "sceneStart",   "position": { "x": 0.0,    "y": 0.0 } },
        { "id": 2,  "type": "showText",     "position": { "x": 240.0,  "y": 0.0 },
          "pinValues": { "textIn": "The guard blocks the gate." } },
        { "id": 3,  "type": "setVariable",  "position": { "x": 480.0,  "y": 0.0 },
          "pinValues": { "name": "gold" } },
        { "id": 4,  "type": "integerValue", "position": { "x": 480.0,  "y": 200.0 },
          "pinValues": { "value": 25 } },
        { "id": 5,  "type": "getVariable",  "position": { "x": 720.0,  "y": 0.0 },
          "pinValues": { "name": "gold" } },
        { "id": 6,  "type": "greater",      "position": { "x": 960.0,  "y": 0.0 } },
        { "id": 7,  "type": "integerValue", "position": { "x": 960.0,  "y": 200.0 },
          "pinValues": { "value": 20 } },
        { "id": 8,  "type": "branch",       "position": { "x": 1200.0, "y": 0.0 } },
        { "id": 9,  "type": "showChoices",  "position": { "x": 1440.0, "y": 0.0 },
          "extraPins": 2,
          "pinValues": { "option0": "Bribe the guard", "option1": "Walk away" } },
        { "id": 10, "type": "showText",     "position": { "x": 1680.0, "y": -120.0 },
          "pinValues": { "textIn": "You bribe your way through." } },
        { "id": 11, "type": "showText",     "position": { "x": 1680.0, "y": 120.0 },
          "pinValues": { "textIn": "You turn back." } },
        { "id": 12, "type": "showText",     "position": { "x": 1440.0, "y": 320.0 },
          "pinValues": { "textIn": "You have too little gold." } },
        { "id": 13, "type": "showText",     "position": { "x": 960.0,  "y": 440.0 },
          "pinValues": { "textIn": "You have no money at all." } }
      ],
      "connections": [
        { "from": 1, "fromPin": "out",      "to": 2,  "toPin": "in" },
        { "from": 2, "fromPin": "out",      "to": 3,  "toPin": "in" },
        { "from": 4, "fromPin": "value",    "to": 3,  "toPin": "value" },
        { "from": 3, "fromPin": "out",      "to": 5,  "toPin": "in" },
        { "from": 5, "fromPin": "out",      "to": 6,  "toPin": "in" },
        { "from": 5, "fromPin": "notFound", "to": 13, "toPin": "in" },
        { "from": 5, "fromPin": "value",    "to": 6,  "toPin": "left" },
        { "from": 7, "fromPin": "value",    "to": 6,  "toPin": "right" },
        { "from": 6, "fromPin": "out",      "to": 8,  "toPin": "in" },
        { "from": 6, "fromPin": "result",   "to": 8,  "toPin": "condition" },
        { "from": 8, "fromPin": "true",     "to": 9,  "toPin": "in" },
        { "from": 8, "fromPin": "false",    "to": 12, "toPin": "in" },
        { "from": 9, "fromPin": "chosen0",  "to": 10, "toPin": "in" },
        { "from": 9, "fromPin": "chosen1",  "to": 11, "toPin": "in" }
      ]
    }
  ]
})";

    struct RecordingHost : loom::Host
    {
        void showText(const std::string& text, const loom::TextStyle&) override
        {
            lines.push_back(text);
        }

        void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle&) override
        {
            offered.clear();
            for (const loom::Option& option : options) offered.push_back(option.text);
        }

        std::vector<std::string> lines;
        std::vector<std::string> offered;
    };

    loom::NodeCatalog builtins()
    {
        loom::NodeCatalog catalog;
        loom::registerBuiltinNodes(catalog);
        return catalog;
    }

    loom::Value storyDocument()
    {
        loom::Value document;
        std::string error;
        REQUIRE(loom::parseJson(kStory, document, error));
        return document;
    }

    loom::Project load(const loom::Value& document, const loom::NodeCatalog& catalog)
    {
        loom::Project project;
        loom::Diagnostics diagnostics;

        REQUIRE(loom::readProject(document, catalog, project, diagnostics));

        for (const loom::Diagnostic& entry : diagnostics.all())
        {
            INFO(entry.message << " (graph " << entry.graph
                               << ", node " << entry.node << ", pin " << entry.pin << ")");
            REQUIRE(entry.severity == loom::Severity::Warning);
        }
        REQUIRE_FALSE(diagnostics.hasErrors());

        return project;
    }

    // Reaches into the parsed document the way an author would reach into the
    // editor: change one pin's value, then play the story again.
    void setPinValue(loom::Value& document, int nodeId, const std::string& pin, loom::Value value)
    {
        for (loom::Value& node : document["graphs"][0]["nodes"])
        {
            if (loom::asInt(node["id"]) == nodeId) node["pinValues"][pin] = std::move(value);
        }
    }
}

TEST_CASE("the story plays to the ending the player chooses", "[story]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::Project project = load(storyDocument(), catalog);

    RecordingHost host;
    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.waiting());
    REQUIRE(host.lines == std::vector<std::string>{ "The guard blocks the gate." });
    REQUIRE(host.offered == std::vector<std::string>{ "Bribe the guard", "Walk away" });

    SECTION("bribing")
    {
        interpreter.choose(0);

        REQUIRE(interpreter.finished());
        REQUIRE(host.lines.back() == "You bribe your way through.");
    }

    SECTION("walking away")
    {
        interpreter.choose(1);

        REQUIRE(interpreter.finished());
        REQUIRE(host.lines.back() == "You turn back.");
    }
}

TEST_CASE("changing one value in the file changes the story", "[story]")
{
    const loom::NodeCatalog catalog = builtins();

    // No recompilation: the gold the author typed is all that decides this.
    loom::Value document = storyDocument();
    setPinValue(document, 4, "value", 10);

    const loom::Project project = load(document, catalog);

    RecordingHost host;
    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
    REQUIRE(host.offered.empty());
    REQUIRE(host.lines.back() == "You have too little gold.");
}

TEST_CASE("a variable that was never written takes the second route", "[story]")
{
    const loom::NodeCatalog catalog = builtins();

    loom::Value document = storyDocument();
    setPinValue(document, 5, "name", "silver");

    const loom::Project project = load(document, catalog);

    RecordingHost host;
    loom::Interpreter interpreter(project, catalog, host);
    interpreter.start();

    REQUIRE(interpreter.finished());
    REQUIRE(host.lines.back() == "You have no money at all.");
}

TEST_CASE("a story saved at the choice restores to the same choice", "[story][save]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::Project project = load(storyDocument(), catalog);

    RecordingHost first;
    loom::Interpreter original(project, catalog, first);
    original.start();
    REQUIRE(original.waiting());

    std::string error;
    loom::Value document;
    REQUIRE(loom::parseJson(loom::writeJson(loom::writeSave(original.save())), document, error));

    loom::SaveState restored;
    REQUIRE(loom::readSave(document, restored, error));

    RecordingHost second;
    loom::Interpreter reloaded(project, catalog, second);
    reloaded.restore(restored);

    REQUIRE(reloaded.waiting());
    reloaded.choose(0);

    REQUIRE(reloaded.finished());
    REQUIRE(second.lines.back() == "You bribe your way through.");

    // The gold that was set before the save is still there afterwards.
    REQUIRE(loom::asInt(reloaded.save().variables.at("gold")) == 25);
}

TEST_CASE("a story survives being written back out and read again", "[story][round trip]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::Project project = load(storyDocument(), catalog);

    const loom::Project reloaded = load(loom::writeProject(project), catalog);

    RecordingHost host;
    loom::Interpreter interpreter(reloaded, catalog, host);
    interpreter.start();
    interpreter.choose(1);

    REQUIRE(interpreter.finished());
    REQUIRE(host.lines.back() == "You turn back.");
}
