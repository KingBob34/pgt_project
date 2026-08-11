#include "node_geometry.h"

#include <QtNodes/AbstractGraphModel>

namespace
{
    constexpr int kMinimumWidth = 160;
}

void NodeGeometry::recomputeSize(QtNodes::NodeId nodeId) const
{
    QtNodes::DefaultHorizontalNodeGeometry::recomputeSize(nodeId);
    QSize computed = size(nodeId);
    if (computed.width() >= kMinimumWidth) return;
    computed.setWidth(kMinimumWidth);

    _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, computed);
}
