#ifndef NODE_RESULT_H
#define NODE_RESULT_H
#include <string>
#include <vector>

// One option offered to the player
struct DisplayOption
{
    std::string label;
    bool available = true;
};

// What executing one node asks the runner to do next
struct NodeResult
{
    enum class Kind
    {
        Continue,
        Show,
        Ask,
        Stop
    };
    Kind kind = Kind::Stop;
    std::string pin;   // Continue, Show
    std::string text;   // Show
    std::vector<DisplayOption> options;   // Ask
    std::vector<std::string> pins;   // Ask: one pin per offered option
    std::string outcome;   // Stop
};

#endif //NODE_RESULT_H
