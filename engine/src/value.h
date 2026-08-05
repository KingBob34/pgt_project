#ifndef VALUE_H
#define VALUE_H
#include <nlohmann/json.hpp>

// A variable's value can be any JSON type (number, bool, string,...)
// This alias is the single place that decides how a runtime value is represented,
// so it can be swapped for a dedicated Value type later without touching callers.

using Value = nlohmann::json;

#endif //VALUE_H
