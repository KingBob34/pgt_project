#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "loom/graph/node.h"

namespace
{
    loom::PinSpec in(std::string name, std::string type)
    {
        return { std::move(name), "", loom::PinDirection::Input, std::move(type) };
    }

    loom::PinSpec out(std::string name, std::string type)
    {
        return { std::move(name), "", loom::PinDirection::Output, std::move(type) };
    }

    // Two inputs of one type, so which of them a wire lands on can be told.
    class CompareNode : public loom::NodeType
    {
    public:
        std::string name()        const override { return "compare"; }
        std::string displayName() const override { return "Compare"; }
        std::string category()    const override { return "Test"; }
        bool        isPure()      const override { return true; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("left", loom::PinType::Int), in("right", loom::PinType::Int),
                     out("result", loom::PinType::Bool) };
        }
    };

    class TextNode : public loom::NodeType
    {
    public:
        std::string name()        const override { return "text"; }
        std::string displayName() const override { return "Text"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("in", loom::PinType::Flow), in("body", loom::PinType::String),
                     out("out", loom::PinType::Flow) };
        }
    };

    // A pin whose type is that of a variable the author has not chosen yet.
    class ReaderNode : public loom::NodeType
    {
    public:
        std::string name()        const override { return "reader"; }
        std::string displayName() const override { return "Reader"; }
        std::string category()    const override { return "Test"; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            loom::PinSpec named{ "name", "", loom::PinDirection::Input,
                                 loom::PinType::VariableName, loom::Value("") };

            loom::PinSpec value{ "value", "", loom::PinDirection::Output,
                                 loom::PinType::Unset, loom::Value(), 0, "name" };

            return { named, value };
        }
    };

    class FrameNode : public loom::NodeType
    {
    public:
        std::string name()        const override { return "frame"; }
        std::string displayName() const override { return "Frame"; }
        std::string category()    const override { return "Test"; }
        bool        isFrame()     const override { return true; }

        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("text", loom::PinType::String) };
        }
    };
}

TEST_CASE("a wire lands on the first pin of its type", "[graph][landing]")
{
    const CompareNode compare;

    REQUIRE(loom::landingPin(compare, loom::PinType::Int, loom::PinDirection::Input) == "left");
    REQUIRE(loom::landingPin(compare, loom::PinType::Bool, loom::PinDirection::Output) == "result");
}

TEST_CASE("a node with no pin of that type takes no wire", "[graph][landing]")
{
    const CompareNode compare;

    REQUIRE(loom::landingPin(compare, loom::PinType::String, loom::PinDirection::Input).empty());
    REQUIRE(loom::landingPin(compare, loom::PinType::Flow, loom::PinDirection::Input).empty());
    REQUIRE(loom::landingPin(compare, loom::PinType::Int, loom::PinDirection::Output).empty());
}

TEST_CASE("the side asked for is the side answered", "[graph][landing]")
{
    const TextNode text;

    REQUIRE(loom::landingPin(text, loom::PinType::Flow, loom::PinDirection::Input) == "in");
    REQUIRE(loom::landingPin(text, loom::PinType::Flow, loom::PinDirection::Output) == "out");
}

TEST_CASE("an any output reaches only an any input", "[graph][landing]")
{
    class AnyNode : public TextNode
    {
    public:
        std::vector<loom::PinSpec> pins(int) const override
        {
            return { in("value", loom::PinType::Any), out("value", loom::PinType::Any) };
        }
    };

    const AnyNode any;
    const CompareNode compare;

    // An Any input is the one place a wire of any type may end.
    REQUIRE(loom::landingPin(any, loom::PinType::Int, loom::PinDirection::Input) == "value");

    // And what it hands on is only ever taken by another Any.
    REQUIRE(loom::landingPin(compare, loom::PinType::Any, loom::PinDirection::Input).empty());
    REQUIRE(loom::landingPin(any, loom::PinType::Any, loom::PinDirection::Output) == "value");
}

TEST_CASE("a pin waiting for its variable is offered to any data wire", "[graph][landing]")
{
    const ReaderNode reader;

    REQUIRE(loom::landingPin(reader, loom::PinType::Int, loom::PinDirection::Output) == "value");
    REQUIRE(loom::landingPin(reader, loom::PinType::Color, loom::PinDirection::Output) == "value");

    REQUIRE(loom::landingPin(reader, loom::PinType::Flow, loom::PinDirection::Output).empty());
}

TEST_CASE("nothing lands on a variable name pin", "[graph][landing]")
{
    const ReaderNode reader;

    REQUIRE(loom::landingPin(reader, loom::PinType::String, loom::PinDirection::Input).empty());
}

TEST_CASE("a frame takes no wire at all", "[graph][landing]")
{
    const FrameNode frame;

    REQUIRE(loom::landingPin(frame, loom::PinType::String, loom::PinDirection::Input).empty());
}
