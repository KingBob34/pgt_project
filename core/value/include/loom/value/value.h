#ifndef LOOM_VALUE_VALUE_H
#define LOOM_VALUE_VALUE_H
#include <nlohmann/json.hpp>

namespace loom
{
    // A variable's value can be any JSON type
    // (number, bool, string, list, object)
    // The single place that decides how a runtime value is represented
    using Value = nlohmann::json;
}

#endif //LOOM_VALUE_VALUE_H
