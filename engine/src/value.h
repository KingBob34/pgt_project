#ifndef VALUE_H
#define VALUE_H
#include <nlohmann/json.hpp>

// A variable's value can be any JSON type (number, bool, string,...)
// The single place that decides how a runtime value is represented.

using Value = nlohmann::json;

#endif //VALUE_H
