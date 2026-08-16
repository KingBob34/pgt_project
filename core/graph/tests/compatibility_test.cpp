#include <catch2/catch_test_macros.hpp>

#include "loom/graph/pin.h"

TEST_CASE("flow connects only to flow", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Flow));

    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Any));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Any, loom::PinType::Flow));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Bool));
}

TEST_CASE("any takes every data type in", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::String, loom::PinType::Any));
    REQUIRE(loom::isCompatible(loom::PinType::Int, loom::PinType::Any));
    REQUIRE(loom::isCompatible(loom::PinType::Any, loom::PinType::Any));
}

TEST_CASE("any hands nothing on but to another any", "[graph][pin]")
{
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Any, loom::PinType::Int));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Any, loom::PinType::String));
}

TEST_CASE("a pin with no variable chosen connects to nothing", "[graph][pin]")
{
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Unset, loom::PinType::Any));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::Unset));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Unset, loom::PinType::Unset));
}

TEST_CASE("data types connect only to themselves", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::Int, loom::PinType::Int));
    REQUIRE(loom::isCompatible(loom::PinType::Color, loom::PinType::Color));

    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::Float));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Float, loom::PinType::Int));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::String));
}

TEST_CASE("nothing at all may be wired into a variable name", "[graph][pin]")
{
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::String, loom::PinType::VariableName));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Any, loom::PinType::VariableName));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::VariableName, loom::PinType::VariableName));
}

TEST_CASE("a variable name pin holds the text of the name", "[graph][pin]")
{
    REQUIRE(loom::canHold(loom::PinType::VariableName, loom::PinType::String));
    REQUIRE_FALSE(loom::canHold(loom::PinType::VariableName, loom::PinType::Int));

    // Anything else is judged by the same rule a wire would be.
    REQUIRE(loom::canHold(loom::PinType::Any, loom::PinType::Int));
    REQUIRE_FALSE(loom::canHold(loom::PinType::Int, loom::PinType::String));
}

TEST_CASE("pinTypeLabel gives the wording used in diagnostics", "[graph][pin]")
{
    REQUIRE(loom::pinTypeLabel(loom::PinType::Int) == "Integer");
    REQUIRE(loom::pinTypeLabel(loom::PinType::Any) == "Any");
    REQUIRE(loom::pinTypeLabel("texture") == "texture");
}
