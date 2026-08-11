#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "loom/nodes/builtin.h"

namespace
{
    struct RecordingHost : loom::Host
    {
        void showText(const std::string& text, const loom::TextStyle& style) override
        {
            lines.push_back(text);
            fontSizes.push_back(style.fontSize);
        }

        void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle&) override
        {
            offered.clear();
            for (const loom::Option& option : options) offered.push_back(option.text);
        }

        void command(const std::string& name, const loom::Value& args) override
        {
            commands.push_back({ name, args });
        }

        struct Command { std::string name; loom::Value args; };

        std::vector<std::string> lines;
        std::vector<long long>   fontSizes;
        std::vector<std::string> offered;
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
    context.inputs["text"] = "the gate is shut";
    context.inputs["fontSize"] = 22;

    const loom::FlowResult result = catalog.find("showText")->execute(context);

    REQUIRE(result.pin == "out");
    REQUIRE(context.recorder.lines == std::vector<std::string>{ "the gate is shut" });
    REQUIRE(context.recorder.fontSizes == std::vector<long long>{ 22 });
    REQUIRE(context.outputs.at("text") == "the gate is shut");
}

TEST_CASE("Get Variable takes a second route when nothing is stored", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::NodeType* get = catalog.find("getVariable");

    FakeContext context;
    context.inputs["name"] = "gold";

    SECTION("missing")
    {
        const loom::FlowResult result = get->execute(context);

        REQUIRE(result.pin == "notFound");
        REQUIRE(context.outputs.find("value") == context.outputs.end());
    }

    SECTION("present")
    {
        context.variables["gold"] = 25;
        const loom::FlowResult result = get->execute(context);

        REQUIRE(result.pin == "out");
        REQUIRE(context.outputs.at("value") == 25);
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

    REQUIRE(equal->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == false);
    REQUIRE_FALSE(reportedError(context.recorder));
}

TEST_CASE("the ordering tests answer for numbers", "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = 3;
    context.inputs["right"] = 25;

    REQUIRE(catalog.find("less")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == true);

    REQUIRE(catalog.find("greater")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == false);

    context.inputs["right"] = 3;
    REQUIRE(catalog.find("lessOrEqual")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == true);

    REQUIRE(catalog.find("greaterOrEqual")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == true);
}

TEST_CASE("the ordering tests report and halt on things that have no order",
          "[nodes][behaviour]")
{
    const loom::NodeCatalog catalog = builtins();

    FakeContext context;
    context.inputs["left"] = "mage";
    context.inputs["right"] = 25;

    const loom::FlowResult result = catalog.find("less")->execute(context);

    REQUIRE(result.kind == loom::FlowResult::Kind::Stop);
    REQUIRE(reportedError(context.recorder));
    REQUIRE(context.outputs.find("result") == context.outputs.end());
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

    REQUIRE(catalog.find("floor")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == 2);

    REQUIRE(catalog.find("ceil")->execute(context).pin == "out");
    REQUIRE(context.outputs.at("result") == 3);

    REQUIRE(catalog.find("round")->execute(context).pin == "out");
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
                                     "boolValue", "colorValue", "comment" })
    {
        INFO("node type: " << type);
        REQUIRE(catalog.find(type)->execute(context).kind == loom::FlowResult::Kind::Stop);
    }
}
