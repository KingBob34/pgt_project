#ifndef STORY_H
#define STORY_H
#include <string>
#include <vector>
#include <map>

// A single comparison
struct Condition
{
    std::string var;   // which variable to check
    std::string op;   // comparison operator: "==", "!=", ">", ">=", "<", "<="
    int value = 0;   //value to compare against
};

// A single change to a variable
struct Effect
{
    std::string var;   // which variable to change
    std::string op;   // assignment operator: "=", "+=", "-="
    int value = 0;   // operand
};

// What to do with a choice whose condition is not met.
enum class ConUnmetDisplay
{
    Hide,   // do not show it at all
    Show   // show it, but marked as locked and unselectable
};

struct Choice
{
    std::string text;
    std::string target;
    bool hasCondition = false;   // does this choice have a condition
    Condition condition;   //only meaningful when hasCondition is true
    ConUnmetDisplay whenUnmet = ConUnmetDisplay::Hide;
    std::vector<Effect> effects;   // applied when this choice is taken (may be empty)
};

struct Passage
{
    std::string id;
    std::string type = "dialogue";   // node type
    std::string text;
    std::vector<Choice> choices;   // empty means this is an ending
};

struct Story
{
    std::string title;
    std::string startId;
    std::map<std::string, int> initialVars;   // variable name -> starting value
    std::map<std::string, Passage> passages;
};

#endif //STORY_H
