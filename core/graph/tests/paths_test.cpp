#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>

#include "loom/graph/graph.h"

namespace
{
    // pet: { dog: { health: 100, tricks: [ "sit" ] } }
    std::map<std::string, loom::VariableSpec> declared()
    {
        loom::Value tricks = loom::Value::array();
        tricks.push_back("sit");

        loom::Value dog = loom::Value::object();
        dog["health"] = 100;
        dog["tricks"] = tricks;

        loom::Value pet = loom::Value::object();
        pet["dog"] = dog;

        loom::VariableSpec pets;
        pets.type = loom::VariableType::Group;
        pets.value = pet;

        loom::VariableSpec gold;
        gold.type = loom::VariableType::Int;
        gold.value = 25;

        return { { "pet", pets }, { "gold", gold } };
    }
}

TEST_CASE("every nested field is offered as a path", "[graph][variables]")
{
    const std::vector<std::string> paths = loom::variablePaths(declared());

    // The groups themselves are walked through, never offered: nothing in the
    // engine can build one, so a node could only ever spoil it.
    REQUIRE(paths == std::vector<std::string>{ "gold", "pet.dog.health", "pet.dog.tricks" });
}

TEST_CASE("the walk stops at a list, whose places are numbered", "[graph][variables]")
{
    const std::vector<std::string> paths = loom::variablePaths(declared());

    for (const std::string& path : paths) REQUIRE(path.find("pet.dog.tricks.") != 0);
}

TEST_CASE("a path carries the type of the field it ends on", "[graph][variables]")
{
    const std::map<std::string, loom::VariableSpec> variables = declared();

    REQUIRE(loom::declaredTypeAt(variables, "gold") == loom::VariableType::Int);
    REQUIRE(loom::declaredTypeAt(variables, "pet.dog.health") == loom::VariableType::Int);
    REQUIRE(loom::declaredTypeAt(variables, "pet.dog.tricks") == loom::VariableType::List);
}

TEST_CASE("a list travels on a pin of its own", "[graph][variables]")
{
    REQUIRE(loom::pinTypeOfVariable(loom::VariableType::List) == loom::PinType::List);

    // Which is what stops a number being written over one.
    REQUIRE_FALSE(loom::isCompatible(loom::PinType::Int, loom::PinType::List));
    REQUIRE(loom::isCompatible(loom::PinType::List, loom::PinType::List));
    REQUIRE(loom::isCompatible(loom::PinType::List, loom::PinType::Any));
}

TEST_CASE("a path that names nothing has no type", "[graph][variables]")
{
    const std::map<std::string, loom::VariableSpec> variables = declared();

    REQUIRE(loom::declaredTypeAt(variables, "silver").empty());
    REQUIRE(loom::declaredTypeAt(variables, "pet.cat").empty());
    REQUIRE(loom::declaredTypeAt(variables, "pet.dog.mood").empty());
    REQUIRE(loom::declaredTypeAt(variables, "gold.somewhere").empty());
}
