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
    bool isNumber(const Value& value) {return value.is_number();}
    bool isInt(const Value& value) {return value.is_number_integer();}
    bool isFloat(const Value& value) {return value.is_number_float();}
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

    const Value* objectGet(const Value& value, const std::string& key)
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

    Value makeObject()
    {
        return Value::object();
    }

    Value makeList()
    {
        return Value::array();
    }

    void objectSet(Value& object, const std::string& key, Value item)
    {
        object[key] = std::move(item);
    }

    std::size_t listSize(const Value& list)
    {
        return list.is_array() ? list.size() : 0;
    }

    bool listContains(const Value& list, const Value& item)
    {
        if (!list.is_array()) return false;

        for (const Value& held : list)
        {
            if (equals(held, item)) return true;
        }

        return false;
    }

    const Value* listAt(const Value& list, std::size_t index)
    {
        if (!list.is_array() || index >= list.size()) return nullptr;

        return &list[index];
    }

    std::vector<const Value*> listItems(const Value& list)
    {
        std::vector<const Value*> items;
        if (!list.is_array()) return items;

        for (const Value& item : list) items.push_back(&item);

        return items;
    }

    void listAppend(Value& list, Value item)
    {
        if (!list.is_array()) return;

        list.push_back(std::move(item));
    }

    bool listRemoveFirst(Value& list, const Value& item)
    {
        if (!list.is_array()) return false;

        for (auto held = list.begin(); held != list.end(); ++held)
        {
            if (!equals(*held, item)) continue;

            list.erase(held);
            return true;
        }

        return false;
    }
}
