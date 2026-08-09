#ifndef EDITOR_SCENE_H
#define EDITOR_SCENE_H
#include <QtNodes/DataFlowGraphicsScene>

// Filters node types out of the right-click menu.
class EditorScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT

public:
    using DataFlowGraphicsScene::DataFlowGraphicsScene;

    QMenu* createSceneMenu(QPointF scenePos) override;
};

#endif //EDITOR_SCENE_H
