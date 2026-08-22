#ifndef LOOM_SERIALIZATION_GRAPH_IO_H
#define LOOM_SERIALIZATION_GRAPH_IO_H
#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

namespace loom
{
    // The file format's own version, independent of the engine's version.
    // A reader that meets a number it does not know refuses the file.
    inline constexpr int kSchemaVersion = 4;

    Value writeProject(const Project& project);

    // Fills Diagnostics as it goes and returns false only when the document is
    // too broken to build anything from at all.
    bool readProject(const Value& document, const NodeCatalog& catalog,
                     Project& out, Diagnostics& diagnostics);
}

#endif //LOOM_SERIALIZATION_GRAPH_IO_H
