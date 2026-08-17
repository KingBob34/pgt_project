#include "loom/value/path.h"
#include "loom/value/inspect.h"

namespace loom
{
    std::vector<std::string> splitPath(const std::string& path)
    {
        std::vector<std::string> segments;
        std::size_t start = 0;
        while (true)
        {
            const std::size_t dot = path.find(".", start);
            if (dot == std::string::npos)
            {
                segments.push_back(path.substr(start));
                break;
            }
            segments.push_back(path.substr(start, dot - start));
            start = dot + 1;
        }
        return segments;
    }

    const Value* descend(const Value& root, const std::vector<std::string>& segments, std::size_t from)
    {
        const Value* current = &root;
        for (std::size_t i = from; i < segments.size(); ++i)
        {
            current = objectGet(*current, segments[i]);
            if (current == nullptr) return nullptr;
        }
        return current;
    }

    bool assign(Value& root, const std::vector<std::string>& segments, Value value, std::size_t from)
    {
        if (from >= segments.size())
        {
            root = std::move(value);
            return true;
        }

        if (!isObject(root)) return false;

        const auto found = root.find(segments[from]);
        if (found == root.end()) return false;

        return assign(*found, segments, std::move(value), from + 1);
    }
}
