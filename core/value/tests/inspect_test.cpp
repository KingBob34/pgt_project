#include <catch2/catch_test_macros.hpp>

#include "loom/value/inspect.h"

TEST_CASE("typeName names every type the pin system knows", "[value][inspect]")
{
    REQUIRE(loom::typeName(loom::Value()) == "null");
    REQUIRE(loom::typeName(true) == "bool");
    REQUIRE(loom::typeName(25) == "int");
    REQUIRE(loom::typeName(2.5) == "float");
    REQUIRE(loom::typeName("hello") == "string");
    REQUIRE(loom::typeName(loom::Value::array()) == "list");
    REQUIRE(loom::typeName(loom::Value::object()) == "object");
}

TEST_CASE("int and float are separate types", "[value][inspect]")
{
    const loom::Value integer = 25;
    const loom::Value fraction = 2.5;

    REQUIRE(loom::isInt(integer));
    REQUIRE_FALSE(loom::isFloat(integer));

    REQUIRE(loom::isFloat(fraction));
    REQUIRE_FALSE(loom::isInt(fraction));

    REQUIRE(loom::isNumber(integer));
    REQUIRE(loom::isNumber(fraction));
}

TEST_CASE("conversions are strict and never throw", "[value][inspect]")
{
    const loom::Value text = "hello";
    const loom::Value integer = 25;
    const loom::Value fraction = 2.5;
    const loom::Value absent;

    SECTION("each reader accepts its own type")
    {
        REQUIRE(loom::asBool(loom::Value(true)) == true);
        REQUIRE(loom::asInt(integer) == 25);
        REQUIRE(loom::asFloat(fraction) == 2.5);
        REQUIRE(loom::asString(text) == "hello");
    }

    SECTION("int and float do not read as each other")
    {
        REQUIRE(loom::asFloat(integer) == 0.0);
        REQUIRE(loom::asInt(fraction) == 0);
    }

    SECTION("any other type reads as the empty value")
    {
        REQUIRE(loom::asBool(text) == false);
        REQUIRE(loom::asInt(text) == 0);
        REQUIRE(loom::asFloat(text) == 0.0);
        REQUIRE(loom::asString(integer).empty());
    }

    SECTION("an absent value reads the same as a wrong type")
    {
        REQUIRE(loom::asBool(absent) == false);
        REQUIRE(loom::asInt(absent) == 0);
        REQUIRE(loom::asFloat(absent) == 0.0);
        REQUIRE(loom::asString(absent).empty());
    }
}

TEST_CASE("toText renders any value for display", "[value][inspect]")
{
    REQUIRE(loom::toText(loom::Value("hello")) == "hello");
    REQUIRE(loom::toText(loom::Value(25)) == "25");
    REQUIRE(loom::toText(loom::Value(2.5)) == "2.5");
    REQUIRE(loom::toText(loom::Value(true)) == "true");
    REQUIRE(loom::toText(loom::Value()).empty());
}

TEST_CASE("toText renders a list and an object too", "[value][inspect]")
{
    loom::Value list = loom::Value::array();
    list.push_back(1);
    list.push_back(2);

    loom::Value object = loom::Value::object();
    object["detail"] = "gone wrong";

    REQUIRE(loom::toText(list) == "[1,2]");
    REQUIRE(loom::toText(object) == "{\"detail\":\"gone wrong\"}");
}

TEST_CASE("objectGet returns nullptr rather than failing", "[value][inspect]")
{
    loom::Value pets = loom::Value::object();
    pets["cat"] = 3;

    REQUIRE(loom::objectGet(pets, "cat") != nullptr);
    REQUIRE(loom::objectGet(pets, "dog") == nullptr);
    REQUIRE(loom::objectGet(loom::Value(25), "cat") == nullptr);
}

TEST_CASE("listAt returns nullptr rather than failing", "[value][inspect]")
{
    loom::Value bag = loom::Value::array();
    bag.push_back("rope");
    bag.push_back("torch");

    REQUIRE(loom::listAt(bag, 0) != nullptr);
    REQUIRE(*loom::listAt(bag, 1) == loom::Value("torch"));
    REQUIRE(loom::listAt(bag, 2) == nullptr);
    REQUIRE(loom::listAt(loom::Value(25), 0) == nullptr);
}
