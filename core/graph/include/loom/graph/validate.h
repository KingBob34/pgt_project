#ifndef LOOM_GRAPH_VALIDATE_H
#define LOOM_GRAPH_VALIDATE_H
#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

namespace loom
{
    // Never throws. An unfinished graph collects warnings and still loads,
    // so the editor can open work in progress.
    void validate(const Graph& graph, const NodeCatalog& catalog, Diagnostics& out);

    void validate(const Project& project, const NodeCatalog& catalog, Diagnostics& out);
}

#endif //LOOM_GRAPH_VALIDATE_H
