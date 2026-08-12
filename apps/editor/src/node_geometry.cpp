#include "node_geometry.h"

#include <QFont>
#include <QFontMetrics>

#include <QtNodes/AbstractGraphModel>

namespace
{
    constexpr int kMinimumWidth = 120;

    // DefaultHorizontalNodeGeometry's own spacing, which it does not expose.
    constexpr int kPortSpacing = 10;
}

int portRowHeight()
{
    return QFontMetrics(QFont()).height() + kPortSpacing;
}

void NodeGeometry::recomputeSize(QtNodes::NodeId nodeId) const
{
    QtNodes::DefaultHorizontalNodeGeometry::recomputeSize(nodeId);
    QSize computed = size(nodeId);
    if (computed.width() >= kMinimumWidth) return;
    computed.setWidth(kMinimumWidth);

    _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, computed);
}
