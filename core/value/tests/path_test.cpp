#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "loom/value/inspect.h"
#include "loom/value/parse.h"
#include "loom/value/path.h"

using Segments = std::vector<std::string>;

TEST_CASE("splitPath splits on dots", "[value][path]")
{
    const Segments one{ "gold" };
    const Segments three{ "pets", "cat", "hunger" };

    REQUIRE(loom::splitPath("gold") == one);
    REQUIRE(loom::splitPath("pets.cat.hunger") == three);
}

TEST_CASE("splitPath keeps empty segments", "[value][path]")
{
    const Segments only{ "" };
    const Segments trailing{ "a", "" };
    const Segments leading{ "", "a" };
    const Segments middle{ "a", "", "b" };

    REQUIRE(loom::splitPath("") == only);
    REQUIRE(loom::splitPath("a.") == trailing);
    REQUIRE(loom::splitPath(".a") == leading);
    REQUIRE(loom::splitPath("a..b") == middle);
}

TEST_CASE("descend walks into nested objects", "[value][path]")
{
    loom::Value root;
    std::string error;
    REQUIRE(loom::parseJson(R"({ "pets": { "cat": { "hunger": 3, "weight": 4.5 } } })", root, error));

    SECTION("a path that exists yields the value")
    {
        const loom::Value* found = loom::descend(root, loom::splitPath("pets.cat.hunger"));

        REQUIRE(found != nullptr);
        REQUIRE(loom::asInt(*found) == 3);
    }

    SECTION("descend preserves the value's own type")
    {
        const loom::Value* hunger = loom::descend(root, loom::splitPath("pets.cat.hunger"));
        const loom::Value* weight = loom::descend(root, loom::splitPath("pets.cat.weight"));

        REQUIRE(hunger != nullptr);
        REQUIRE(weight != nullptr);

        REQUIRE(loom::isInt(*hunger));
        REQUIRE(loom::isFloat(*weight));

        REQUIRE(loom::asFloat(*weight) == 4.5);
        REQUIRE(loom::asFloat(*hunger) == 0.0);
        REQUIRE(loom::asInt(*weight) == 0);
    }

    SECTION("a missing key yields nullptr")
    {
        REQUIRE(loom::descend(root, loom::splitPath("pets.dog.hunger")) == nullptr);
    }

    SECTION("stepping into a non-object yields nullptr")
    {
        REQUIRE(loom::descend(root, loom::splitPath("pets.cat.hunger.more")) == nullptr);
    }

    SECTION("from skips leading segments")
    {
        const loom::Value* pets = loom::objectGet(root, "pets");
        REQUIRE(pets != nullptr);

        const loom::Value* found = loom::descend(*pets, loom::splitPath("pets.cat.hunger"), 1);

        REQUIRE(found != nullptr);
        REQUIRE(loom::asInt(*found) == 3);
    }
}
