#include "engine.h"
#include "terminal_io.h"
#include <string>
#include <vector>
#include  <map>
#include <set>
#include <sstream>

namespace
{
    // The mutable state of one playthrough: which variables hold which values
    struct GameState
    {
        std::map<std::string, int> variables;
        int get(const std::string& name) const
        {
            auto it = variables.find(name);
            if (it != variables.end())
            {
                return it -> second;
            }
            return 0;
        }
        void set(const std::string& name, int value)
        {
            variables[name] = value;
        }
    };

    // Evaluate one condition against the current state
    bool evaluate(const Condition& condition, const GameState& state)
    {
        int left = state.get(condition.var);
        int right = condition.value;
        const std::string& op = condition.op;

        if (op == "==") return left == right;
        if (op == "!=") return left != right;
        if (op == ">")  return left >  right;
        if (op == ">=") return left >= right;
        if (op == "<")  return left <  right;
        if (op == "<=") return left <= right;

        return false;
    }

    // Apply one effect to the state
    void applyEffect(const Effect& effect, GameState& state)
    {
        int current = state.get(effect.var);
        const std::string& op = effect.op;
        if (op ==  "=") state.set(effect.var, effect.value);
        else if (op == "+=") state.set(effect.var, current + effect.value);
        else if (op == "-=") state.set(effect.var, current - effect.value);
    }

    // Answer a player query command like "?gold"
    std::string queryVariable(const std::string& cmd, const GameState& state)
    {
        std::string name = cmd.substr(1);   // drop the leading "?"
        if (name.empty())
        {
            if (state.variables.empty())
            {
                return "No variables.";
            }
            std::string result = "Variables:";
            for (const auto& [key, value] : state.variables)
            {
                result += "\n " + key + " = " + std::to_string(value);
            }
            return result;
        }
        auto it = state.variables.find(name);
        if (it != state.variables.end())
        {
            return name + " = " + std::to_string(it -> second);
        }
        return "Unknown variable: " + name;
    }

        // Recursive helper: render one node and everything reachable from it.
    void renderNode(const Story& story,
                    const std::string& id,
                    const std::string& prefix,
                    bool isLast,
                    const std::string& edgeLabel,
                    std::set<std::string>& ancestors,
                    std::string& out)
    {
        // The connector for this node's own line.
        std::string branch = isLast ? "\\-- " : "|-- ";

        std::string line = prefix + branch;
        if (!edgeLabel.empty()) {
            line += "[" + edgeLabel + "] ";
        }
        line += id;

        // Missing target: report and stop (loader should have caught it, but be safe).
        auto it = story.passages.find(id);
        if (it == story.passages.end()) {
            out += line + " (MISSING)\n";
            return;
        }
        const Passage& passage = it->second;

        if (passage.choices.empty()) {
            out += line + " (end)\n";
            return;
        }

        // Cycle guard: if this id is already on the current path, stop here.
        if (ancestors.count(id) > 0) {
            out += line + " (loop)\n";
            return;
        }

        out += line + "\n";

        ancestors.insert(id);
        std::string childPrefix = prefix + (isLast ? "    " : "|   ");
        for (int i = 0; i < static_cast<int>(passage.choices.size()); i++) {
            const Choice& choice = passage.choices[i];
            bool childIsLast = (i == static_cast<int>(passage.choices.size()) - 1);
            renderNode(story, choice.target, childPrefix, childIsLast, choice.text, ancestors, out);
        }
        ancestors.erase(id);
    }

    // Entry point: render the whole story as a tree from its start.
    std::string renderStoryTree(const Story& story)
    {
        std::string out;
        std::set<std::string> ancestors;
        renderNode(story, story.startId, "", true, "", ancestors, out);
        return out;
    }

}

void runStory(const Story& story)
{
    GameState state;
    state.variables = story.initialVars;
    std::string currentId = story.startId;

    while (true)
    {
        const Passage& passage = story.passages.at(currentId);

        // Build the list of options the player will actually see.
        std::vector<DisplayOption> display;
        std::vector<int> choiceIndex;   // maps a display slot back to its Choice
        for (int i = 0; i < static_cast<int>(passage.choices.size()); i++)
        {
            const Choice& choice = passage.choices[i];
            bool available = true;
            if (choice.hasCondition)
            {
                available = evaluate(choice.condition, state);
            }
            if (available)
            {
                display.push_back({choice.text, true});
                choiceIndex.push_back(i);
            } else if (choice.whenUnmet == ConUnmetDisplay::Show)
            {
                display.push_back({"", false});
                choiceIndex.push_back(i);
            }
            // Hide: add nothing
        }

        showPassage(passage.text, display);
        if (passage.choices.empty())
        {
            break;
        }

        int slot = -1;
        while (slot < 0)
        {
            std::string input = readLine("\n> ");

            //Trim leading/trailing whitespace.
            size_t start = input.find_first_not_of(" \t");
            if (start == std::string::npos)
            {
                continue;   // blank line, ask again
            }
            size_t end = input.find_last_not_of(" \t");
            input = input.substr(start, end - start + 1);
            if (input[0] == '?') {
                showMessage(queryVariable(input, state));
                continue;
            }
            if (input[0] == '/') {
                if (input == "/path") {
                    showMessage(renderStoryTree(story));
                } else {
                    showMessage("Unknown command: " + input);
                }
                continue;
            }
            // Otherwise, treat it as an option number.
            int entered = 0;
            try {
                entered = std::stoi(input);
            } catch (const std::exception&) {
                showMessage("Please enter the number of an available option, ?var, or /path.");
                continue;
            }
            int index = entered - 1;
            if (index >= 0 && index < static_cast<int>(display.size()) && display[index].available) {
                slot = index;
            } else {
                showMessage("Please enter the number of an available option.");
            }
        }
        const Choice& chosen = passage.choices[choiceIndex[slot]];
        for (const Effect& effect : chosen.effects)
        {
            applyEffect(effect, state);
        }
        currentId = chosen.target;
    }
}
