#include "loom/graph/catalog.h"

namespace loom
{
    void NodeCatalog::add(std::unique_ptr<NodeType> type)
    {
        if (type == nullptr) return;

        const std::string key = type->name();
        types[key] = std::move(type);
    }

    const NodeType* NodeCatalog::find(const std::string& name) const
    {
        const auto found = types.find(name);
        if (found == types.end()) return nullptr;

        return found->second.get();
    }

    std::vector<const NodeType*> NodeCatalog::all() const
    {
        std::vector<const NodeType*> result;
        result.reserve(types.size());

        for (const auto& entry : types)
        {
            result.push_back(entry.second.get());
        }

        return result;
    }
}
