#include <catch2/catch_test_macros.hpp>

#include <string>

#include "loom/value/parse.h"

TEST_CASE("parseJson reports malformed input with a message", "[value][parse]")
{
    loom::Value out;
    std::string error;

    REQUIRE_FALSE(loom::parseJson(R"({ "gold": })", out, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("parseJson clears a stale error on success", "[value][parse]")
{
    loom::Value out;
    std::string error = "left over from an earlier call";

    REQUIRE(loom::parseJson(R"({ "gold": 25 })", out, error));
    REQUIRE(error.empty());
}

TEST_CASE("a Value survives a write and parse round trip", "[value][parse]")
{
    const std::string text = R"({ "meta": { "title": "Gate" }, "gold": 25, "flags": [true, null] })";

    loom::Value original;
    std::string error;
    REQUIRE(loom::parseJson(text, original, error));

    loom::Value reparsed;
    REQUIRE(loom::parseJson(loom::writeJson(original), reparsed, error));

    REQUIRE(reparsed == original);
}
