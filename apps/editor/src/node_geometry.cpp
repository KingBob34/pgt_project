#include "node_geometry.h"

#include <QFont>
#include <QFontMetrics>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/DataFlowGraphModel>

#include "node_adaptor.h"

namespace
{
    constexpr int kMinimumWidth = 120;

    // DefaultHorizontalNodeGeometry's own spacing, which it does not expose.
    constexpr int kPortSpacing = 10;

    constexpr int kRowGap = 6;
}

int portRowHeight()
{
    return QFontMetrics(QFont()).height() + kPortSpacing;
}

int portRowGap()
{
    return kRowGap;
}

void NodeGeometry::recomputeSize(QtNodes::NodeId nodeId) const
{
    QtNodes::DefaultHorizontalNodeGeometry::recomputeSize(nodeId);
    QSize computed = size(nodeId);
    if (computed.width() >= kMinimumWidth) return;
    computed.setWidth(kMinimumWidth);

    _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, computed);
}

const NodeAdaptor* NodeGeometry::adaptorFor(QtNodes::NodeId nodeId) const
{
    QtNodes::DataFlowGraphModel* flow = dynamic_cast<QtNodes::DataFlowGraphModel*>(&_graphModel);

    return flow == nullptr ? nullptr : flow->delegateModel<NodeAdaptor>(nodeId);
}

QPointF NodeGeometry::portPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                                   QtNodes::PortIndex index) const
{
    const QPointF base =
        QtNodes::DefaultHorizontalNodeGeometry::portPosition(nodeId, portType, index);

    const NodeAdaptor* adaptor = adaptorFor(nodeId);
    if (adaptor == nullptr) return base;

    // Replaces the base class's even spacing with the editor rows' own.
    const double even = index * portRowHeight() + portRowHeight() / 2.0;
    const double ours = adaptor->rowTop(portType, index)
                        + adaptor->rowHeight(portType, index) / 2.0;

    return QPointF(base.x(), base.y() - even + ours);
}
