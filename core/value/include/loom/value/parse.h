#ifndef LOOM_VALUE_PARSE_H
#define LOOM_VALUE_PARSE_H

#include <string>
#include "loom/value/value.h"

// The boundary between JSON text and Value.
// The only place that parses or serialises.
namespace loom
{
    // On failure returns false and writes the parser's message into error
    bool parseJson(const std::string& text, Value& out, std::string& error);

    // Two-space indent: project files are meant to stay readable and diffable
    std::string writeJson(const Value& value);
}

#endif //LOOM_VALUE_PARSE_H
