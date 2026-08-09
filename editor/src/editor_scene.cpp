#include "editor_scene.h"
#include <QMenu>
#include <QTreeWidget>

QMenu* EditorScene::createSceneMenu(QPointF const scenePos)
{
    QMenu* menu = QtNodes::DataFlowGraphicsScene::createSceneMenu(scenePos);

    // The start node is created with the graph, never added by hand
    for (QTreeWidget* tree : menu->findChildren<QTreeWidget*>())
    {
        const auto entries = tree->findItems("start", Qt::MatchExactly | Qt::MatchRecursive);
        for (QTreeWidgetItem* entry : entries)
        {
            delete entry;
        }
    }

    return menu;
}
