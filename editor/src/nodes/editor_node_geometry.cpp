#include "editor_node_geometry.h"
#include "editor_node.h"
#include <QSize>
#include <QtNodes/DataFlowGraphModel>

void EditorNodeGeometry::recomputeSize(QtNodes::NodeId const nodeId) const
{
    DefaultHorizontalNodeGeometry::recomputeSize(nodeId);

    // Each node type supplies its own floor
    int minimumWidth = kDefaultNodeWidth;
    if (auto* dataFlowModel = dynamic_cast<QtNodes::DataFlowGraphModel*>(&_graphModel))
    {
        if (auto* node = dataFlowModel->delegateModel<EditorNode>(nodeId))
        {
            minimumWidth = node->minimumWidth();
        }
    }

    QSize size = _graphModel.nodeData<QSize>(nodeId, QtNodes::NodeRole::Size);
    if (size.width() >= minimumWidth) return;

    size.setWidth(minimumWidth);
    _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, size);
}
