#ifndef LOOM_SERIALIZATION_GRAPH_IO_H
#define LOOM_SERIALIZATION_GRAPH_IO_H
#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

namespace loom
{
    // The file format's own version, independent of the engine's version.
    // A reader that meets a number it does not know refuses the file.
    inline constexpr int kSchemaVersion = 3;

    Value writeGraph(const Graph& graph);
    Value writeProject(const Project& project);

    // Both fill Diagnostics as they go and return false only when the document
    // is too broken to build anything from at all.
    bool readGraph(const Value& document, const NodeCatalog& catalog,
                   Graph& out, Diagnostics& diagnostics);

    bool readProject(const Value& document, const NodeCatalog& catalog,
                     Project& out, Diagnostics& diagnostics);
}

#endif //LOOM_SERIALIZATION_GRAPH_IO_H
