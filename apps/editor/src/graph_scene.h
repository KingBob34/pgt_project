#ifndef LOOM_EDITOR_GRAPH_SCENE_H
#define LOOM_EDITOR_GRAPH_SCENE_H
#include <QtNodes/DataFlowGraphicsScene>

#include "loom/graph/catalog.h"

class GraphScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT

public:
    GraphScene(QtNodes::DataFlowGraphModel& model, const loom::NodeCatalog& catalog,
               QObject* parent = nullptr);

    QMenu* createSceneMenu(QPointF scenePos) override;

private:
    const loom::NodeCatalog& catalog;
};

#endif //LOOM_EDITOR_GRAPH_SCENE_H
