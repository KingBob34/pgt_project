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

        // A passage the author wrote: styled runs of text, and slots standing
        // for the node value pins. Written only in the editor, never wired.
        inline constexpr const char* Prose  = "prose";

        // Names one of the story's declared globals. Chosen from a list of
        // them, never computed, so no wire may reach it.
        inline constexpr const char* VariableName = "variableName";

        // Names one of the story's own scenes, chosen the same way and for the
        // same reason: a scene that is picked from a list cannot be mistyped.
        inline constexpr const char* SceneName = "sceneName";

        // A pin that follows a variable nothing has been chosen for yet.
        inline constexpr const char* Unset = "unset";
    }

    // How much room a String pin's editor needs, declared by the node that
    // knows what it is asking the author for.
    namespace TextShape
    {
        inline constexpr int Line  = 0;   // a word or a number
        inline constexpr int Label = 1;   // a sentence, in a small box
    }

    struct PinSpec
    {
        std::string  name;   // identity in the file: "true", "option0"
        std::string  label;   // shown in the editor: "True", "Option 1"
        PinDirection direction = PinDirection::Input;
        std::string  type = PinType::Flow;
        Value        defaultValue;   // starting in-place value of a data input
        int          textShape = TextShape::Line;

        // Names the variableName pin whose choice decides this pin's type.
        // Empty on every pin whose type is fixed.
        std::string typeFollows;

        // The editor keeps this value but shows no row and no connector for
        // it: it is the shape of the node, not something the story reads.
        bool hidden = false;
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
