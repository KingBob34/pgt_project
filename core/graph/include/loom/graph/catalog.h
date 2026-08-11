#ifndef LOOM_GRAPH_CATALOG_H
#define LOOM_GRAPH_CATALOG_H
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "loom/graph/node.h"

namespace loom
{
    // The node types a project may use. Owned and passed by value, never global.
    class NodeCatalog
    {
    public:
        void add(std::unique_ptr<NodeType> type);

        const NodeType* find(const std::string& name) const;

        // Sorted by type name. Used to build the editor's node menu.
        std::vector<const NodeType*> all() const;

    private:
        std::map<std::string, std::unique_ptr<NodeType>> types;
    };
}

#endif //LOOM_GRAPH_CATALOG_H
