#ifndef LOOM_VALUE_INSPECT_H
#define LOOM_VALUE_INSPECT_H
#include <cstddef>
#include <string>
#include <vector>
#include "loom/value/value.h"

// Everything the engine is allowed to do to a Value.
// No other layer may call the underlying JSON library directly.
namespace loom
{
    // "null" / "bool" / "int" / "float" / "string" / "list" / "object".
    // The wording used in Diagnostics messages
    std::string typeName(const Value& value);

    bool isNull(const Value& value);
    bool isBool(const Value& value);
    bool isNumber(const Value& value);
    bool isInt(const Value& value);
    bool isFloat(const Value& value);
    bool isString(const Value& value);
    bool isList(const Value& value);
    bool isObject(const Value& value);

    // Conversions never throw. A value of the wrong type, or an absent one,
    // reads as the empty value of the type asked for.
    bool asBool(const Value& value);
    long long asInt (const Value& value);
    double asFloat (const Value& value);

    // Presentation, not reading: any value rendered as text
    std::string toText(const Value& value);

    // Comparison across any two values, for the == < <= > >= nodes.
    // Numbers compare as numbers, strings lexicographically; anything else
    // has no order, so every ordering question about it answers false.
    bool equals(const Value& left, const Value& right);
    bool lessThan(const Value& left, const Value& right);

    // Refuses anything that is not already a string
    std::string asString(const Value& value);

    // Returns nullptr when the value is not an object or has no such key.
    const Value* objectGet(const Value& value, const std::string& key);

    // The keys of an object, in the order the library keeps them.
    // With objectGet, this is the only way to walk an object from outside.
    std::vector<std::string> objectKeys(const Value& value);

    // List handling. Anything that is not a list counts as empty, and the two
    // that change one leave a value of another kind alone.
    std::size_t listSize(const Value& list);
    bool listContains(const Value& list, const Value& item);
    void listAppend(Value& list, Value item);

    // Returns nullptr when the value is not a list or is shorter than that.
    // With listSize, this is the only way to walk a list from outside.
    const Value* listAt(const Value& list, std::size_t index);

    // Drops the first item equal to this one. False when there was none.
    bool listRemoveFirst(Value& list, const Value& item);
}

#endif //LOOM_VALUE_INSPECT_H
