#ifndef CONDITION_H
#define CONDITION_H
#include <string>
#include <vector>
#include "value.h"
#include "game_state.h"

// What kind of test one node of a condition expression performs
enum class ConditionKind
{
    Comparison,   // leaf, compare one variable against a value
    All,   // compound, true when every child is true (AND)
    Any,   // compound, true when any child is true (OR)
    Not   // compound, true when its single child is false (NOT)
};

// One node of a condition expression tree
// A Comparison uses var/op/value and has no children.
// All/Any/Not use children only, and leave var/op/value empty.
struct Condition
{
    ConditionKind kind = ConditionKind::Comparison;
    std::string var;   // name of the variable to test
    std::string op;   // comparing operator
    Value value;   // the value to compare against
    std::vector<Condition> children;   // operands of All/Any/Not
};

// Build a Condition tree from its JSON form
Condition parseCondition(const Value& json);

// Evaluate whether a condition holds for the given variables
bool evaluate(const Condition& condition, const GameState& state);

#endif //CONDITION_H
