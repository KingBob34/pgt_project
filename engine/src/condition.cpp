#include "condition.h"
#include <stdexcept>
#include <algorithm>

namespace
{
    bool isKnownOperator(const std::string& op)
    {
        return op == "==" || op == "!=" ||
                op == ">" || op == ">=" ||
                op == "<"  || op == "<=" ||
                op == "contains";
    }

    // Parses the operand array of an all/any compound
    std::vector<Condition> parseOperands(const Value& json)
    {
        if (!json.is_array() || json.empty())
        {
            throw std::runtime_error("'all' and 'any' need a non-empty array of conditions");
        }

        std::vector<Condition> operands;
        operands.reserve(json.size());
        for (const Value& operand : json)
        {
            operands.push_back(parseCondition(operand));
        }
        return operands;
    }

    // Compares two values with one of the leaf operators
    bool compare(const Value& left, const std::string& op, const Value& right)
    {
        if (op == "==") return left == right;
        if (op == "!=") return left != right;
        if (op == "contains")
        {
            if (!left.is_array())
            {
                throw std::runtime_error("'contains' needs a list variable");
            }
            return  std::find(left.begin(), left.end(), right) != left.end();
        }
        if (!left.is_number() || !right.is_number())
        {
            throw std::runtime_error("'" + op + "' needs numbers on both sides");
        }

        const double a = left.get<double>();
        const double b = right.get<double>();
        if (op == ">") return a > b;
        if (op == ">=") return a >= b;
        if (op == "<") return a < b;
        if (op == "<=") return  a <= b;
        throw std::runtime_error("unknown operator: '" + op + "'");
    }
}

Condition parseCondition(const Value& json)
{
    if (!json.is_object())
    {
        throw std::runtime_error("a condition must be a JSON object");
    }
    Condition condition;
    if (json.contains("all"))
    {
        condition.kind = ConditionKind::All;
        condition.children = parseOperands(json.at("all"));
    }
    else if (json.contains("any"))
    {
        condition.kind = ConditionKind::Any;
        condition.children = parseOperands(json.at("any"));
    }
    else if (json.contains("not"))
    {
        condition.kind = ConditionKind::Not;
        condition.children.push_back(parseCondition(json.at("not")));
    }
    else
    {
        condition.kind = ConditionKind::Comparison;
        condition.var = json.at("var").get<std::string>();
        condition.op = json.at("op").get<std::string>();
        condition.value = json.at("value");
        if (!isKnownOperator(condition.op))
        {
            throw std::runtime_error("unknown condition operator: '" + condition.op + "'");
        }
    }
    return condition;
}

// Evaluate whether a condition holds for the given variables
bool evaluate(const Condition& condition, const GameState& state)
{
    switch (condition.kind)
    {
    case ConditionKind::Comparison:
        {
            const Value* value = state.find(condition.var);
            if (value == nullptr)
            {
                throw std::runtime_error("condition reads undeclared variable '" + condition.var + "'");
            }
            return compare(*value, condition.op, condition.value);
        }
    case ConditionKind::All:
        for (const Condition& child : condition.children)
        {
            if (!evaluate(child, state)) return false;
        }
        return true;
    case ConditionKind::Any:
        for (const Condition& child : condition.children)
        {
            if (evaluate(child, state)) return true;
        }
        return false;
    case ConditionKind::Not:
        return !evaluate(condition.children.at(0), state);
    }
    throw std::runtime_error("unhandled condition kind");
}