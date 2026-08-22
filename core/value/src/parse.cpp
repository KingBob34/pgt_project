#include "loom/value/parse.h"

namespace loom
{
    bool parseJson(const std::string& text, Value& out, std::string& error)
    {
        try
        {
            out = Value::parse(text);
        }
        catch (const Value::parse_error& failure)
        {
            error = failure.what();
            return false;
        }

        error.clear();
        return true;
    }

    std::string writeJson(const Value& value)
    {
        return value.dump(2);
    }
}
