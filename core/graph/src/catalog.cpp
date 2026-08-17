#include "loom/graph/catalog.h"

#include <algorithm>

namespace loom
{
    void NodeCatalog::add(std::unique_ptr<NodeType> type)
    {
        if (type == nullptr) return;

        const std::string key = type->name();
        NodeType* arriving = type.get();

        // A second type under one name takes the first one's place in the
        // order rather than appearing twice.
        const auto found = types.find(key);

        if (found == types.end()) added.push_back(arriving);
        else                      std::replace(added.begin(), added.end(), found->second.get(), arriving);

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
        return std::vector<const NodeType*>(added.begin(), added.end());
    }
}
