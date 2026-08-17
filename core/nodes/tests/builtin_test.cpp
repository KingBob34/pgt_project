#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>

#include "loom/nodes/builtin.h"

#include "loom/graph/validate.h"
#include "loom/value/inspect.h"

namespace
{
    loom::NodeCatalog builtins()
    {
        loom::NodeCatalog catalog;
        loom::registerBuiltinNodes(catalog);
        return catalog;
    }

    const loom::PinSpec* findPin(const std::vector<loom::PinSpec>& pins, const std::string& name)
    {
        for (const loom::PinSpec& pin : pins)
        {
            if (pin.name == name) return &pin;
        }

        return nullptr;
    }
}

TEST_CASE("every builtin node type is registered once", "[nodes][builtin]")
{
    // add() keys on name(), so a duplicated name would silently shrink the
    // catalog and nothing else would notice. Raise this when a node is added.
    const std::size_t expected = 36;

    const loom::NodeCatalog catalog = builtins();

    REQUIRE(catalog.all().size() == expected);

    std::set<std::string> names;
    for (const loom::NodeType* type : catalog.all()) names.insert(type->name());

    REQUIRE(names.size() == expected);
}

TEST_CASE("exactly one builtin node type is an entry point", "[nodes][builtin]")
{
    const loom::NodeCatalog catalog = builtins();

    int entryPoints = 0;
    for (const loom::NodeType* type : catalog.all())
    {
        if (type->isEntryPoint()) ++entryPoints;
    }

    REQUIRE(entryPoints == 1);
    REQUIRE(catalog.find("sceneStart")->isEntryPoint());
}

TEST_CASE("every node declares a display name, a category and unique pin names",
          "[nodes][builtin]")
{
    const loom::NodeCatalog catalog = builtins();

    for (const loom::NodeType* type : catalog.all())
    {
        INFO("node type: " << type->name());

        REQUIRE_FALSE(type->displayName().empty());
        REQUIRE_FALSE(type->category().empty());
        REQUIRE(type->minExtraPins() <= type->maxExtraPins());

        const std::vector<loom::PinSpec> pins = type->pins(type->minExtraPins());

        std::set<std::string> pinNames;
        for (const loom::PinSpec& pin : pins)
        {
            REQUIRE_FALSE(pin.name.empty());
            REQUIRE(pinNames.insert(pin.name).second);
            REQUIRE_FALSE(loom::pinTypeLabel(pin.type).empty());
        }
    }
}

TEST_CASE("a data input's default value matches the type it declares", "[nodes][builtin]")
{
    const loom::NodeCatalog catalog = builtins();

    // validate() rejects a stored value whose type disagrees with its pin, so a
    // default that disagrees would make a freshly created node fail to load.
    for (const loom::NodeType* type : catalog.all())
    {
        for (const loom::PinSpec& pin : type->pins(type->minExtraPins()))
        {
            if (pin.type == loom::PinType::Flow) continue;
            if (loom::isNull(pin.defaultValue)) continue;

            INFO("node type: " << type->name() << ", pin: " << pin.name);
            REQUIRE(loom::canHold(pin.type, loom::typeName(pin.defaultValue)));
        }
    }
}

TEST_CASE("node types keep their pin names across extra pin counts", "[nodes][builtin]")
{
    const loom::NodeCatalog catalog = builtins();
    const loom::NodeType* choices = catalog.find("showChoices");

    REQUIRE(choices != nullptr);
    REQUIRE(choices->maxExtraPins() > choices->minExtraPins());

    const std::vector<loom::PinSpec> few = choices->pins(2);
    const std::vector<loom::PinSpec> many = choices->pins(5);

    REQUIRE(many.size() > few.size());

    // An option nobody has wired a condition to is one the player can pick.
    const loom::PinSpec* enabled = findPin(few, "enabled0");

    REQUIRE(enabled != nullptr);
    REQUIRE(enabled->type == loom::PinType::Bool);
    REQUIRE(loom::asBool(enabled->defaultValue));

    // Growing the option count must not rename or retype what was already there.
    for (const loom::PinSpec& pin : few)
    {
        const loom::PinSpec* same = findPin(many, pin.name);

        INFO("pin: " << pin.name);
        REQUIRE(same != nullptr);
        REQUIRE(same->type == pin.type);
        REQUIRE(same->direction == pin.direction);
    }
}

TEST_CASE("a graph built from real node types validates cleanly", "[nodes][builtin]")
{
    const loom::NodeCatalog catalog = builtins();

    loom::NodeInstance text;
    text.id = 2;
    text.type = "showText";
    text.pinValues["textIn"] = "the gate is shut";

    loom::NodeInstance start;
    start.id = 1;
    start.type = "sceneStart";

    loom::NodeInstance print;
    print.id = 3;
    print.type = "print";

    loom::Graph graph;
    graph.name = "gate";
    graph.nodes = { start, text, print };
    graph.connections = { { 1, "out", 2, "in" },
                          { 2, "out", 3, "in" },
                          { 2, "textOut", 3, "value" } };

    loom::Diagnostics diagnostics;
    loom::validate(graph, catalog, diagnostics);

    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        INFO(entry.message << " (node " << entry.node << ", pin " << entry.pin << ")");
        REQUIRE(entry.severity == loom::Severity::Warning);
    }

    REQUIRE_FALSE(diagnostics.hasErrors());
}
