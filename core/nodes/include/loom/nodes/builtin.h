#ifndef LOOM_NODES_BUILTIN_H
#define LOOM_NODES_BUILTIN_H
#include "loom/graph/catalog.h"

namespace loom
{
    // Fills a catalog with every node type the engine ships with.
    void registerBuiltinNodes(NodeCatalog& catalog);
}

#endif //LOOM_NODES_BUILTIN_H
