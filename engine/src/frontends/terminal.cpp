#include "frontends/terminal.h"
#include <iostream>
#include <stdexcept>

namespace
{
    std::string trim(const std::string& text)
    {
        const size_t first = text.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        const size_t last = text.find_last_not_of(" \t");
        return text.substr(first, last - first + 1);
    }

    // Ask the player to pick one of the offered options
    bool askForChoice(Runner& runner, const std::vector<DisplayOption>& options)
    {
        while (true)
        {
            const std::optional<std::string> input = readLine("\n> ");
            if (!input.has_value())
            {
                showMessage("\nInput ended.");
                return false;
            }
            const std::string line = trim(input.value());
            if (line.empty()) continue;

            int number = 0;
            try {number = std::stoi(line);} catch (const std::exception&)
            {
                showMessage("Please enter the number of an available option.");
                continue;
            }

            const int index = number - 1;
            if (index < 0 || index >= static_cast<int>(options.size())|| !options[index].available)
            {
                showMessage("Please enter the number of an available option.");
                continue;
            }
            runner.choose(index);
            return true;
        }
    }
}

void showText(const std::string& text)
{
    std::cout << "\n" << text << "\n";
}

void showOptions(const std::vector<DisplayOption>& options)
{
    std::cout << "\n";
    int number = 1;
    for (const DisplayOption& option : options)
    {
        if (option.available)
        {
            std::cout << "  " << number << ") " << option.label << "\n";
        } else
        {
            std::cout << "  " << number << ") [LOCKED]\n";
        }
        number++;
    }
}

std::optional<std::string> readLine(const std::string& prompt)
{
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return line;
}

void showMessage(const std::string& text)
{
    std::cout << text << "\n";
}

void runInTerminal(Runner& runner)
{
    while (true)
    {
        const Step step = runner.advance();
        if (step.kind == Step::Kind::ShowText)
        {
            showText(step.text);
            continue;
        }
        if (step.kind == Step::Kind::Finished)
        {
            showMessage("\n[ ending: "  + step.outcome + " ]");
            return;
        }
        showOptions(step.options);
        if (!askForChoice(runner, step.options)) return;
    }
}
