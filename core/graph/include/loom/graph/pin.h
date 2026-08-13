#ifndef LOOM_GRAPH_PIN_H
#define LOOM_GRAPH_PIN_H
#include <string>
#include "loom/value/value.h"

namespace loom
{
    enum class PinDirection { Input, Output };

    // Pin type ids. "flow" is a type like any other; there is no separate kind field.
    namespace PinType
    {
        inline constexpr const char* Flow   = "flow";
        inline constexpr const char* Any    = "object";
        inline constexpr const char* Bool   = "bool";
        inline constexpr const char* Int    = "int";
        inline constexpr const char* Float  = "float";
        inline constexpr const char* String = "string";
        inline constexpr const char* Color  = "color";
    }

    struct PinSpec
    {
        std::string  name;   // identity in the file: "true", "option0"
        std::string  label;   // shown in the editor: "True", "Option 1"
        PinDirection direction = PinDirection::Input;
        std::string  type = PinType::Flow;
        Value        defaultValue;   // starting in-place value of a data input
        bool longText = false;   // holds prose rather than a single line
    };

    // Whether a wire may join a pin of type 'from' to a pin of type 'to'.
    bool isCompatible(const std::string& from, const std::string& to);

    // Pin type wording for diagnostics: "Integer", "String", etc
    std::string pinTypeLabel(const std::string& type);
}

#endif //LOOM_GRAPH_PIN_H
