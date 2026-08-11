#include <catch2/catch_test_macros.hpp>

#include "loom/graph/pin.h"

TEST_CASE("flow connects only to flow", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Flow));

    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Any));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Any, loom::PinType::Flow));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Flow, loom::PinType::Bool));
}

TEST_CASE("any connects to every data type", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::Any, loom::PinType::Int));
    REQUIRE(loom::isCompatible(loom::PinType::String, loom::PinType::Any));
    REQUIRE(loom::isCompatible(loom::PinType::Any, loom::PinType::Any));
}

TEST_CASE("data types connect only to themselves", "[graph][pin]")
{
    REQUIRE(loom::isCompatible(loom::PinType::Int, loom::PinType::Int));
    REQUIRE(loom::isCompatible(loom::PinType::Color, loom::PinType::Color));

    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::Float));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Float, loom::PinType::Int));
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::String));
}

TEST_CASE("pinTypeLabel gives the wording used in diagnostics", "[graph][pin]")
{
    REQUIRE(loom::pinTypeLabel(loom::PinType::Int) == "Integer");
    REQUIRE(loom::pinTypeLabel(loom::PinType::Any) == "Any");
    REQUIRE(loom::pinTypeLabel("texture") == "texture");
}
