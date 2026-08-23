#include <catch2/catch_test_macros.hpp>

#include <string>

#include "loom/graph/prose.h"
#include "loom/value/inspect.h"

namespace
{
    loom::Value colour(double red, double green, double blue)
    {
        loom::Value made = loom::makeObject();

        loom::objectSet(made, "r", red);
        loom::objectSet(made, "g", green);
        loom::objectSet(made, "b", blue);

        return made;
    }

    // A passage of one span, which is all these cases need.
    loom::Value passageOf(loom::Value span)
    {
        loom::Value spans = loom::makeList();
        loom::listAppend(spans, std::move(span));

        loom::Value made = loom::makeObject();
        loom::objectSet(made, "spans", spans);

        return made;
    }

    std::string reads(int) { return "25"; }
}

TEST_CASE("a slot reads the pin it stands for", "[graph][prose]")
{
    loom::Value span = loom::makeObject();
    loom::objectSet(span, "slot", 0);

    const std::vector<loom::TextRun> runs = loom::prose::runs(passageOf(span), reads);

    REQUIRE(runs.size() == 1);
    REQUIRE(runs.front().text == "25");
}

TEST_CASE("a slot carries the look the author gave it", "[graph][prose]")
{
    loom::Value span = loom::makeObject();
    loom::objectSet(span, "slot", 0);
    loom::objectSet(span, "size", 22);
    loom::objectSet(span, "bold", true);
    loom::objectSet(span, "color", colour(0.8, 0.1, 0.1));

    const std::vector<loom::TextRun> runs = loom::prose::runs(passageOf(span), reads);

    REQUIRE(runs.size() == 1);

    // The value a pin hands over is written into the passage like any other
    // words, so it is styled like any other words.
    REQUIRE(runs.front().text == "25");
    REQUIRE(runs.front().size == 22);
    REQUIRE(runs.front().bold);
    REQUIRE_FALSE(loom::isNull(runs.front().color));
}

TEST_CASE("words the author typed carry their own look", "[graph][prose]")
{
    loom::Value span = loom::makeObject();
    loom::objectSet(span, "text", "the gate is shut");
    loom::objectSet(span, "italic", true);

    const std::vector<loom::TextRun> runs = loom::prose::runs(passageOf(span), reads);

    REQUIRE(runs.size() == 1);
    REQUIRE(runs.front().text == "the gate is shut");
    REQUIRE(runs.front().italic);
}

TEST_CASE("a pin that was never opened says nothing", "[graph][prose]")
{
    REQUIRE(loom::prose::runs(loom::Value(), reads).empty());
}
