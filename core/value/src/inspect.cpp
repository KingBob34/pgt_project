#include "loom/value/inspect.h"

namespace loom
{
    std::string typeName(const Value& value)
    {
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "float";
        if (value.is_string()) return "string";
        if (value.is_array()) return "list";
        if (value.is_object()) return "object";
        return "null";
    }

    bool isNull(const Value& value) {return value.is_null();}
    bool isBool(const Value& value) {return value.is_boolean();}
    bool isInt(const Value& value) {return value.is_number_integer();}
    bool isFloat(const Value& value) {return value.is_number_float();}
    bool isNumber(const Value& value) {return value.is_number();}
    bool isString(const Value& value) {return value.is_string();}
    bool isList(const Value& value) {return value.is_array();}
    bool isObject(const Value& value) {return value.is_object();}

    bool asBool(const Value& value)
    {
        return value.is_boolean() ? value.get<bool>() : false;
    }

    long long asInt(const Value& value)
    {
        return value.is_number_integer() ? value.get<long long>() : 0;
    }

    double asFloat(const Value& value)
    {
        return value.is_number_float() ? value.get<double>() : 0.0;
    }

    std::string toText(const Value& value)
    {
        if (value.is_string()) return value.get<std::string>();
        if (value.is_null()) return std::string();
        return value.dump();
    }

    bool equals(const Value& left, const Value& right)
    {
        return left == right;
    }

    bool lessThan(const Value& left, const Value& right)
    {
        if (left.is_number() && right.is_number())
        {
            return left.get<double>() < right.get<double>();
        }

        if (left.is_string() && right.is_string())
        {
            return left.get<std::string>() < right.get<std::string>();
        }

        return false;
    }

    std::string asString(const Value& value)
    {
        return value.is_string() ? value.get<std::string>() : std::string();
    }

    const Value*  objectGet(const Value& value, const std::string& key)
    {
        if (!value.is_object()) return nullptr;

        const auto found = value.find(key);
        if (found == value.end()) return nullptr;

        return &(*found);
    }

    std::vector<std::string> objectKeys(const Value& value)
    {
        std::vector<std::string> keys;
        if (!value.is_object()) return keys;

        for (auto entry = value.begin(); entry != value.end(); ++entry)
        {
            keys.push_back(entry.key());
        }

        return keys;
    }
}
