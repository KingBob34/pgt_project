#include "node_geometry.h"

#include <algorithm>

#include <QFont>
#include <QFontMetricsF>
#include <QMarginsF>

#include <QtNodes/AbstractGraphModel>

#include "node_adaptor.h"
#include "node_metrics.h"

void NodeGeometry::recomputeSize(QtNodes::NodeId nodeId) const
{
    // A frame is as big as the author dragged it, not as big as its contents.
    if (const NodeAdaptor* frame = adaptorFor(_graphModel, nodeId); frame && frame->isFrame())
    {
        _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, frame->boxSize());
        return;
    }

    QtNodes::DefaultHorizontalNodeGeometry::recomputeSize(nodeId);

    QSize computed = size(nodeId);

    // The base class puts the ports on the edge and sizes the card to the rows
    // alone. Setting them inside costs that much width, but only on a side
    // that has connectors on it: a value node asks for nothing, so the room
    // its missing input side would need is room the card does not want.
    computed.setWidth(std::max(computed.width() + inset(nodeId, QtNodes::PortType::In)
                                                + inset(nodeId, QtNodes::PortType::Out),
                               metrics::minimumWidth));

    // The rows all move down out of the title strip, and the card grows to
    // keep the space it used to have underneath them.
    computed.setHeight(computed.height() + metrics::bodyGap);

    _graphModel.setNodeData(nodeId, QtNodes::NodeRole::Size, computed);
}

int NodeGeometry::inset(QtNodes::NodeId nodeId, QtNodes::PortType portType) const
{
    const QtNodes::NodeRole counted = portType == QtNodes::PortType::Out
                                    ? QtNodes::NodeRole::OutPortCount
                                    : QtNodes::NodeRole::InPortCount;

    return _graphModel.nodeData(nodeId, counted).toUInt() == 0 ? 0 : metrics::portInset;
}

QRectF NodeGeometry::boundingRect(QtNodes::NodeId nodeId) const
{
    const QSize card = size(nodeId);

    return QRectF(QPointF(0.0, 0.0), card)
        .marginsAdded(QMarginsF(metrics::cardMargin, metrics::cardMargin,
                                metrics::cardMargin, metrics::cardMargin));
}

QPointF NodeGeometry::portPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                                   QtNodes::PortIndex index) const
{
    const QPointF base =
        QtNodes::DefaultHorizontalNodeGeometry::portPosition(nodeId, portType, index);

    const NodeAdaptor* adaptor = adaptorFor(_graphModel, nodeId);
    if (adaptor == nullptr) return base;

    // Replaces the base class's even spacing with the editor rows' own.
    const double even = index * metrics::rowHeight() + metrics::rowHeight() / 2.0;
    const double ours = adaptor->rowTop(portType, index)
                        + adaptor->rowHeight(portType, index) / 2.0;

    const double inwards = portType == QtNodes::PortType::In ? inset(nodeId, portType)
                                                             : -inset(nodeId, portType);

    return QPointF(base.x() + inwards, base.y() - even + ours + metrics::bodyGap);
}

QPointF NodeGeometry::portTextPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                                       QtNodes::PortIndex index) const
{
    const QPointF base =
        QtNodes::DefaultHorizontalNodeGeometry::portTextPosition(nodeId, portType, index);

    const double inwards = portType == QtNodes::PortType::In
                         ? inset(nodeId, portType) + metrics::portTextGapIn
                         : -(inset(nodeId, portType) + metrics::portTextGapOut);

    // The base class puts the baseline a quarter of the text's height below the
    // port. Centred on it by the font's own ascent and descent instead, so the
    // name lines up with the middle of the editor it is naming.
    const QFontMetricsF metrics{ QFont() };
    const double centred = portPosition(nodeId, portType, index).y()
                         + (metrics.ascent() - metrics.descent()) / 2.0;

    return QPointF(base.x() + inwards, centred);
}

QPointF NodeGeometry::widgetPosition(QtNodes::NodeId nodeId) const
{
    const QPointF base = QtNodes::DefaultHorizontalNodeGeometry::widgetPosition(nodeId);

    // A node with no editors is given the null point, which is not a position.
    if (base.isNull()) return base;

    return QPointF(base.x() + inset(nodeId, QtNodes::PortType::In),
                   base.y() + metrics::bodyGap);
}
