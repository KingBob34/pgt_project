#ifndef LOOM_EDITOR_GRAPH_DOCUMENT_H
#define LOOM_EDITOR_GRAPH_DOCUMENT_H
#include <string>
#include <utility>

#include "loom/graph/catalog.h"
#include "loom/graph/graph.h"

class GraphModel;

// The graph being edited. The canvas holds the nodes and the wires; the scene
// name and the meta block live here.
class GraphDocument
{
public:
    GraphDocument(GraphModel& model, const loom::NodeCatalog& catalog);

    // Empties the canvas and leaves a single entry point on it.
    void reset();

    void        open(const loom::Graph& graph);
    loom::Graph graph() const;

    const std::string& name() const { return sceneName; }
    void               setName(std::string value) { sceneName = std::move(value); }

    const loom::Meta& meta() const { return sceneMeta; }
    void              setMeta(loom::Meta value) { sceneMeta = std::move(value); }

private:
    void clear();

    GraphModel&              model;
    const loom::NodeCatalog& catalog;

    std::string sceneName = "scene";
    loom::Meta  sceneMeta;
};

#endif //LOOM_EDITOR_GRAPH_DOCUMENT_H
