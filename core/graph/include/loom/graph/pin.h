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
        inline constexpr const char* List   = "list";

        // Names one of the story's declared globals. Chosen from a list of
        // them, never computed, so no wire may reach it.
        inline constexpr const char* VariableName = "variableName";

        // A pin that follows a variable nothing has been chosen for yet.
        inline constexpr const char* Unset = "unset";
    }

    struct PinSpec
    {
        std::string  name;   // identity in the file: "true", "option0"
        std::string  label;   // shown in the editor: "True", "Option 1"
        PinDirection direction = PinDirection::Input;
        std::string  type = PinType::Flow;
        Value        defaultValue;   // starting in-place value of a data input
        bool longText = false;   // holds prose rather than a single line

        // Names the variableName pin whose choice decides this pin's type.
        // Empty on every pin whose type is fixed.
        std::string typeFollows;
    };

    // Whether a wire may join a pin of type 'from' to a pin of type 'to'.
    bool isCompatible(const std::string& from, const std::string& to);

    // Whether a value stored on the node itself may sit in a pin of this type.
    // A different question from isCompatible, which is only about wires.
    bool canHold(const std::string& pinType, const std::string& valueType);

    // Pin type wording for diagnostics: "Integer", "String", etc
    std::string pinTypeLabel(const std::string& type);
}

#endif //LOOM_GRAPH_PIN_H
