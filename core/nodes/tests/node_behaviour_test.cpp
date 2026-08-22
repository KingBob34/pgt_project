#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "loom/nodes/builtin.h"

#include "loom/graph/prose.h"
#include "loom/value/inspect.h"

namespace
{
    struct RecordingHost : loom::Host
    {
        void showText(const std::vector<loom::TextRun>& passage) override
        {
            std::string words;

            for (const loom::TextRun& run : passage) words += run.text;

            lines.push_back(words);
            runCounts.push_back(static_cast<int>(passage.size()));
        }

        void askChoice(const std::vector<loom::Option>& options) override
        {
            offered.clear();
            locked.clear();

            for (const loom::Option& option : options)
            {
                offered.push_back(option.text);
                if (!option.enabled) locked.push_back(option.text);
            }
        }

        void command(const std::string& name, const loom::Value& args) override
        {
            commands.push_back({ name, args });
        }

        struct Command { std::string name; loom::Value args; };

        std::vector<std::string> lines;
        std::vector<int>         runCounts;
        std::vector<std::string> offered;
        std::vector<std::string> locked;
        std::vector<Command>     commands;
    };

    // A context with no graph behind it: whatever the test puts in "inputs" is
    // what the node reads. Nodes depend on this interface and nothing else,
    // which is why they can be exercised without the runtime at all.
    class FakeContext : public loom::ExecutionContext
    {
    public:
        loom::Value input(const std::string& pin) const override
        {
            const auto found = inputs.find(pin);
            return found == inputs.end() ? loom::Value() : found->second;
        }

        void setOutput(const std::string& pin, loom::Value value) override
        {
            outputs[pin] = std::move(value);
        }

        bool readVariable(const std::string& name, loom::Value& out) const override
        {
            const auto found = variables.find(name);
            if (found == variables.end()) return false;

            out = found->second;
            return true;
        }

        void writeVariable(const std::string& name, loom::Value value) override
        {
            variables[name] = std::move(value);
        }

        loom::Host& host() override { return recorder; }

        std::map<std::string, loom::Value> inputs;
        std::map<std::string, loom::Value> outputs;
        std::map<std::string, loom::Value> variables;
        RecordingHost                      recorder;
    };

    loom::NodeCatalog builtins()
    {
        loom::NodeCatalog catalog;
        loom::registerBuiltinNodes(catalog);
        return catalog;
    }

    bool reportedError(const RecordingHost& host)
    {
        for (const RecordingHost::Command& command : host.commands)
        {
            if (command.name == "error") return true;
        }

        return false;
    }
}

TEST_CASE("Branch leaves through the pin the condition names", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::NodeType* branch = catalog.find("branch");

    FakeContext context;

    SECTION("true")
    {
        context.inputs["condition"] = true;
        const loom::FlowResult result = branch->execute(context);

        REQUIRE(result.kind == loom::FlowResult::Kind::Continue);
        REQUIRE(result.pin == "true");
    }

    SECTION("false")
    {
        context.inputs["condition"] = false;
        const loom::FlowResult result = branch->execute(context);

        REQUIRE(result.pin == "false");
    }
}

TEST_CASE("Show Text pushes the words out and hands them on as data", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["textIn"] = loom::prose::fromPlain("the gate is shut");

    const loom::FlowResult result = catalog.find("showText")->execute(context);

    REQUIRE(result.pin == "out");
    REQUIRE(context.recorder.lines == std::vector<std::string>{ "the gate is shut" });
}

TEST_CASE("a slot in a passage reads the pin it stands for", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    loom::Value spans = loom::Value::array();
    spans.push_back(loom::Value{ { "text", "you have " } });
    spans.push_back(loom::Value{ { "slot", 0 } });
    spans.push_back(loom::Value{ { "text", " coins" } });

    FakeContext context;
    context.inputs["textIn"] = loom::Value{ { "spans", spans } };

    // Anything a pin can hold reaches the page as text, not only a string.
    context.inputs["value0"] = 25;

    catalog.find("showText")->execute(context);

    REQUIRE(context.recorder.lines == std::vector<std::string>{ "you have 25 coins" });

    // What the author styled apart stays apart on the way out.
    REQUIRE(context.recorder.runCounts == std::vector<int>{ 3 });
}

TEST_CASE("Get Variable takes a second route when nothing is stored", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::NodeType* get = catalog.find("getVariable");

    FakeContext context;
    context.inputs["name"] = "gold";

    SECTION("missing")
    {
        // Only reachable by deleting a declared variable a node still names,
        // so it is a fault rather than a route the story can take.
        get->execute(context);

        REQUIRE(reportedError(context.recorder));
        REQUIRE(loom::isNull(context.outputs.at("value")));
    }

    SECTION("present")
    {
        context.variables["gold"] = 25;
        get->execute(context);

        REQUIRE(context.outputs.at("value") == 25);
        REQUIRE_FALSE(reportedError(context.recorder));
    }
}

TEST_CASE("Set Variable creates a name nobody has used", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["name"] = "gold";
    context.inputs["value"] = 25;

    REQUIRE(catalog.find("setVariable")->execute(context).pin == "out");
    REQUIRE(context.variables.at("gold") == 25);
}

TEST_CASE("Show Choices leaves out empty options and keeps the pins aligned",
          "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["option0"] = "bribe the guard";
    context.inputs["option1"] = "";
    context.inputs["option2"] = "walk away";

    const loom::FlowResult result = catalog.find("showChoices")->execute(context);

    REQUIRE(result.kind == loom::FlowResult::Kind::Choose);
    REQUIRE(context.recorder.offered ==
            std::vector<std::string>{ "bribe the guard", "walk away" });

    // The player picks the second button they can see, which is option 2's pin.
    REQUIRE(result.optionPins == std::vector<std::string>{ "chosen0", "chosen2" });
}

TEST_CASE("Show Choices shows a locked option but does not let it be picked",
          "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["option0"] = "bribe the guard";
    context.inputs["enabled0"] = false;
    context.inputs["option1"] = "walk away";
    context.inputs["enabled1"] = true;

    const loom::FlowResult result = catalog.find("showChoices")->execute(context);

    REQUIRE(context.recorder.offered ==
            std::vector<std::string>{ "bribe the guard", "walk away" });
    REQUIRE(context.recorder.locked == std::vector<std::string>{ "bribe the guard" });

    // A locked option keeps its place, so the routes still line up with it.
    REQUIRE(result.optionPins == std::vector<std::string>{ "chosen0", "chosen1" });
}

TEST_CASE("End stops the story and names the ending it reached", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["text"] = "The gate closes behind you.";

    const loom::FlowResult result = catalog.find("end")->execute(context);

    REQUIRE(result.kind == loom::FlowResult::Kind::Stop);
    REQUIRE(context.recorder.commands.size() == 1);
    REQUIRE(context.recorder.commands.front().name == "ending");
    REQUIRE(loom::asString(*loom::objectGet(context.recorder.commands.front().args, "text")) ==
            "The gate closes behind you.");
}

TEST_CASE("Go To Scene reports where to continue and never returns", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["scene"] = "village";

    const loom::FlowResult result = catalog.find("goToScene")->execute(context);

    REQUIRE(result.kind == loom::FlowResult::Kind::Jump);
    REQUIRE(result.targetGraph == "village");
}

TEST_CASE("== compares any two values without faulting", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::NodeType* equal = catalog.find("equal");

    FakeContext context;
    context.inputs["left"] = "mage";
    context.inputs["right"] = 25;

    equal->execute(context);
    REQUIRE(context.outputs.at("result") == false);
    REQUIRE_FALSE(reportedError(context.recorder));
}

TEST_CASE("the ordering tests answer for numbers", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = 3;
    context.inputs["right"] = 25;

    catalog.find("less")->execute(context);
    REQUIRE(context.outputs.at("result") == true);

    catalog.find("greater")->execute(context);
    REQUIRE(context.outputs.at("result") == false);

    context.inputs["right"] = 3;
    catalog.find("lessOrEqual")->execute(context);
    REQUIRE(context.outputs.at("result") == true);

    catalog.find("greaterOrEqual")->execute(context);
    REQUIRE(context.outputs.at("result") == true);
}

TEST_CASE("the ordering tests report and halt on things that have no order",
          "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = "mage";
    context.inputs["right"] = 25;

    catalog.find("less")->execute(context);

    REQUIRE(reportedError(context.recorder));
    REQUIRE(context.outputs.at("result") == false);
}

TEST_CASE("Random Integer stays inside its range", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["min"] = 3;
    context.inputs["max"] = 6;

    for (int attempt = 0; attempt < 200; ++attempt)
    {
        REQUIRE(catalog.find("randomInteger")->execute(context).pin == "out");

        const long long drawn = loom::asInt(context.outputs.at("result"));
        REQUIRE(drawn >= 3);
        REQUIRE(drawn <= 6);
    }
}

TEST_CASE("Random Integer reports a reversed range instead of fixing it",
          "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["min"] = 6;
    context.inputs["max"] = 3;

    const loom::FlowResult result = catalog.find("randomInteger")->execute(context);

    REQUIRE(result.kind == loom::FlowResult::Kind::Stop);
    REQUIRE(reportedError(context.recorder));
    REQUIRE(context.outputs.find("result") == context.outputs.end());
}

TEST_CASE("the rounding nodes each choose a different whole number", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["value"] = 2.5;

    catalog.find("floor")->execute(context);
    REQUIRE(context.outputs.at("result") == 2);

    catalog.find("ceil")->execute(context);
    REQUIRE(context.outputs.at("result") == 3);

    catalog.find("round")->execute(context);
    REQUIRE(context.outputs.at("result") == 3);
}

TEST_CASE("Print goes to the console, never to the player's text", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["value"] = 25;

    REQUIRE(catalog.find("print")->execute(context).pin == "out");
    REQUIRE(context.recorder.lines.empty());
    REQUIRE(context.recorder.commands.size() == 1);
    REQUIRE(context.recorder.commands.front().name == "print");
}

TEST_CASE("value nodes and Comment never run", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;

    for (const std::string& type : { "stringValue", "integerValue", "floatValue",
                                     "boolValue", "comment" })
    {
        INFO("node type: " << type);
        REQUIRE(catalog.find(type)->execute(context).kind == loom::FlowResult::Kind::Stop);
    }
}

TEST_CASE("To Float widens a whole number", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["value"] = 3;

    catalog.find("toFloat")->execute(context);

    // asFloat alone reads a whole number as zero, which is the trap here.
    REQUIRE(loom::isFloat(context.outputs["result"]));
    REQUIRE(loom::asFloat(context.outputs["result"]) == 3.0);
}

TEST_CASE("To Float keeps a decimal and counts a Bool", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    {
        FakeContext context;
        context.inputs["value"] = 2.5;

        catalog.find("toFloat")->execute(context);
        REQUIRE(loom::asFloat(context.outputs["result"]) == 2.5);
    }

    {
        FakeContext context;
        context.inputs["value"] = true;

        catalog.find("toFloat")->execute(context);
        REQUIRE(loom::asFloat(context.outputs["result"]) == 1.0);
    }
}

TEST_CASE("To Float reports something that is not a number", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["value"] = "twelve";

    catalog.find("toFloat")->execute(context);

    // Pure, so there is no flow to halt: it says so and answers zero.
    REQUIRE(reportedError(context.recorder));
    REQUIRE(loom::asFloat(context.outputs.at("result")) == 0.0);
}

TEST_CASE("To String renders every kind of value", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    loom::Value list = loom::Value::array();
    list.push_back(1);

    const std::vector<std::pair<loom::Value, std::string>> cases = {
        { loom::Value("already text"), "already text" },
        { loom::Value(42),             "42" },
        { loom::Value(true),           "true" },
        { list,                        "[1]" },
    };

    for (const auto& entry : cases)
    {
        FakeContext context;
        context.inputs["value"] = entry.first;

        catalog.find("toString")->execute(context);
        REQUIRE(loom::asString(context.outputs["result"]) == entry.second);
    }
}

TEST_CASE("To Bool counts a number and reads the two words", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    const std::vector<std::pair<loom::Value, bool>> cases = {
        { loom::Value(true),    true },
        { loom::Value(0),       false },
        { loom::Value(7),       true },
        { loom::Value(0.0),     false },
        { loom::Value("true"),  true },
        { loom::Value("false"), false },
    };

    for (const auto& entry : cases)
    {
        FakeContext context;
        context.inputs["value"] = entry.first;

        catalog.find("toBool")->execute(context);
        REQUIRE(loom::asBool(context.outputs["result"]) == entry.second);
    }
}

TEST_CASE("To Bool reports text that says neither", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["value"] = "maybe";

    catalog.find("toBool")->execute(context);

    REQUIRE(reportedError(context.recorder));
    REQUIRE(loom::asBool(context.outputs.at("result")) == false);
}

TEST_CASE("Divide keeps the fraction it was given", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = 7;
    context.inputs["right"] = 2;

    catalog.find("divide")->execute(context);

    // Two whole numbers in, and the half is still there on the way out.
    REQUIRE(loom::isFloat(context.outputs["result"]));
    REQUIRE(loom::asFloat(context.outputs["result"]) == 3.5);
}

TEST_CASE("Divide reports a division by zero and answers zero", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = 5;
    context.inputs["right"] = 0;

    catalog.find("divide")->execute(context);

    REQUIRE(context.recorder.commands.size() == 1);
    REQUIRE(context.recorder.commands.front().name == "error");
    REQUIRE(loom::asFloat(context.outputs["result"]) == 0.0);
}
