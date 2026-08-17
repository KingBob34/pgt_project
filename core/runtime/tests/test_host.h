#ifndef LOOM_RUNTIME_TESTS_TEST_HOST_H
#define LOOM_RUNTIME_TESTS_TEST_HOST_H
#include <string>
#include <vector>

#include "loom/graph/execution.h"

// A front end that records instead of showing. Its existence is the proof
// that the core never assumes a window, a terminal, or any other output.
class TestHost : public loom::Host
{
public:
    void showText(const std::string& text, const loom::TextStyle&) override
    {
        lines.push_back(text);
    }

    void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle&) override
    {
        offered.clear();
        locked.clear();

        for (const loom::Option& option : options)
        {
            offered.push_back(option.text);
            if (!option.enabled) locked.push_back(option.text);
        }
    }

    void command(const std::string& name, const loom::Value& args) override
    {
        commands.push_back({ name, args });
    }

    struct Command
    {
        std::string name;
        loom::Value args;
    };

    std::vector<std::string> lines;
    std::vector<std::string> offered;
    std::vector<std::string> locked;
    std::vector<Command>     commands;
};

#endif //LOOM_RUNTIME_TESTS_TEST_HOST_H
