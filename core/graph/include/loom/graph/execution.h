#ifndef LOOM_GRAPH_EXECUTION_H
#define LOOM_GRAPH_EXECUTION_H
#include <string>
#include <vector>

#include "loom/value/value.h"

namespace loom
{
    // What a node reports after running.
    struct FlowResult
    {
        enum class Kind
        {
            Continue,   // leave through pin
            Wait,       // hand control back, resume through pin later
            Choose,     // hand control back, resume through optionPins[chosen]
            Jump,       // continue in targetGraph
            Stop        // the story ends here
        };

        Kind                     kind = Kind::Stop;
        std::string              pin;
        std::vector<std::string> optionPins;
        std::string              targetGraph;

        static FlowResult continueOn(std::string pin);
        static FlowResult wait(std::string pin);
        static FlowResult choose(std::vector<std::string> pins);
        static FlowResult jump(std::string graph);
        static FlowResult stop();
    };

    struct TextStyle
    {
        long long fontSize = 16;
        Value     color;
    };

    struct Option
    {
        std::string text;

        // Shown either way; a disabled one cannot be picked yet.
        bool enabled = true;
    };

    // Implemented by the player, by the terminal and by the test double.
    struct Host
    {
        virtual ~Host() = default;

        virtual void showText(const std::string& text, const TextStyle& style) = 0;
        virtual void askChoice(const std::vector<Option>& options, const TextStyle& style) = 0;
        virtual void command(const std::string& /*name*/, const Value& /*args*/) {}
    };

    // The only channel between a node and everything outside it.
    class ExecutionContext
    {
    public:
        virtual ~ExecutionContext() = default;

        virtual Value input(const std::string& pin) const = 0;
        virtual void  setOutput(const std::string& pin, Value value) = 0;

        virtual bool readVariable(const std::string& name, Value& out) const = 0;
        virtual void writeVariable(const std::string& name, Value value) = 0;

        virtual Host& host() = 0;

        bool        inputBool  (const std::string& pin) const;
        long long   inputInt   (const std::string& pin) const;
        double      inputFloat (const std::string& pin) const;
        std::string inputString(const std::string& pin) const;
    };
}

#endif //LOOM_GRAPH_EXECUTION_H
