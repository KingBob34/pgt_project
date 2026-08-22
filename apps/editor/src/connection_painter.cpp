#include "connection_painter.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QPainter>
#include <QPainterPathStroker>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/BasicGraphicsScene.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/internal/ConnectionState.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "canvas_faults.h"
#include "node_adaptor.h"
#include "node_metrics.h"
#include "node_palette.h"

namespace
{
    constexpr int    kHaloAlpha = 130;
    constexpr double kHaloWidth = 7.0;
    constexpr double kStrokeWidth = 10.0;

    // How many straight pieces the curve is chopped into to build a shape that
    // can be clicked on.
    constexpr int kStrokeSegments = 20;

    // How far a loose end reaches for a port. The same distance the connect
    // itself allows, so a wire that has turned green will really land.
    constexpr double kSnapReach = 16.0;

    // The two handles that give a wire its S bend. QtNodes keeps this rule to
    // itself, and a wire has to be drawn between two points that are not
    // always the pair the connection is holding.
    std::pair<QPointF, QPointF> handlesFor(const QPointF& from, const QPointF& to)
    {
        constexpr double kReach = 200.0;

        const double across = to.x() - from.x();

        double sideways = std::min(kReach, std::abs(across));
        double upright = 0.0;
        double share = 0.5;

        if (across <= 0.0)
        {
            const double down = to.y() - from.y() + 20.0;

            upright = std::min(kReach, std::abs(down)) * (down < 0.0 ? -1.0 : 1.0);
            share = 1.0;
        }

        sideways *= share;

        return { QPointF(from.x() + sideways, from.y() + upright),
                 QPointF(to.x() - sideways, to.y() - upright) };
    }

    // The port a loose end would land on, if it would land on one. Only ports
    // the model would actually accept are offered.
    bool reachedPort(const QtNodes::ConnectionGraphicsObject& cgo, QPointF& landing)
    {
        const QtNodes::ConnectionState& state = cgo.connectionState();
        const QtNodes::NodeId over = state.lastHoveredNode();
        const QtNodes::PortType wanted = state.requiredPort();

        if (over == QtNodes::InvalidNodeId || wanted == QtNodes::PortType::None) return false;

        QtNodes::BasicGraphicsScene* graph = cgo.nodeScene();
        if (graph == nullptr) return false;

        QtNodes::NodeGraphicsObject* ngo = graph->nodeGraphicsObject(over);
        if (ngo == nullptr) return false;

        QtNodes::AbstractGraphModel& model = graph->graphModel();

        const QtNodes::NodeRole counted = wanted == QtNodes::PortType::Out
                                        ? QtNodes::NodeRole::OutPortCount
                                        : QtNodes::NodeRole::InPortCount;

        const unsigned int ports = model.nodeData(over, counted).toUInt();
        const QPointF loose = cgo.endPoint(wanted);

        double nearest = kSnapReach;
        bool found = false;

        for (unsigned int port = 0; port < ports; ++port)
        {
            const QtNodes::PortIndex index = static_cast<QtNodes::PortIndex>(port);

            if (!model.connectionPossible(
                    QtNodes::makeCompleteConnectionId(cgo.connectionId(), over, index)))
            {
                continue;
            }

            const QPointF at = cgo.mapFromScene(
                graph->nodeGeometry().portScenePosition(over, wanted, index,
                                                        ngo->sceneTransform()));

            const QPointF away = at - loose;
            const double gap = std::sqrt(QPointF::dotProduct(away, away));

            if (gap >= nearest) continue;

            nearest = gap;
            landing = at;
            found = true;
        }

        return found;
    }

    // A wire carries what its source pin sends, so it takes that pin's colour.
    QColor colourOf(const QtNodes::ConnectionGraphicsObject& cgo)
    {
        const QtNodes::ConnectionId wire = cgo.connectionId();

        const QtNodes::NodeDataType carried =
            cgo.graphModel()
               .portData(wire.outNodeId, QtNodes::PortType::Out, wire.outPortIndex,
                         QtNodes::PortRole::DataType)
               .value<QtNodes::NodeDataType>();

        return palette::pin(carried.id.toStdString());
    }
}

QPainterPath wirePath(const QPointF& from, const QPointF& to)
{
    const auto handles = handlesFor(from, to);

    QPainterPath curve(from);
    curve.cubicTo(handles.first, handles.second, to);

    return curve;
}

QPainterPath wirePath(const QtNodes::ConnectionGraphicsObject& cgo)
{
    return wirePath(cgo.endPoint(QtNodes::PortType::Out), cgo.endPoint(QtNodes::PortType::In));
}

ConnectionPainter::ConnectionPainter(const CanvasFaults& canvasFaults)
    : faults(canvasFaults)
{
}

void ConnectionPainter::paint(QPainter* painter,
                              const QtNodes::ConnectionGraphicsObject& cgo) const
{
    const QtNodes::ConnectionState& state = cgo.connectionState();
    const QPainterPath curve = wirePath(cgo);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);

    // Behind the wire rather than over it, so the halo reads as a mark on the
    // wire and the wire keeps its own colour.
    if (blamed(cgo))
    {
        QColor halo = palette::fault();
        halo.setAlpha(kHaloAlpha);

        painter->setPen(Qt::NoPen);
        painter->setBrush(halo);
        painter->drawPath(getPainterStroke(cgo));
        painter->setBrush(Qt::NoBrush);
    }

    if (cgo.isSelected() || state.hovered())
    {
        painter->setPen(QPen(palette::border(cgo.isSelected()), kHaloWidth));
        painter->drawPath(curve);
    }

    // One end is still under the pointer, so there is no source pin to take a
    // colour from and the wire is drawn as the sketch it is. Where it would
    // land it goes green and reaches the rest of the way itself.
    if (state.requiresPort() || state.frozen())
    {
        QPointF landing;
        const bool willLand = reachedPort(cgo, landing);
        const QtNodes::PortType loose = state.requiredPort();

        const QPointF from = willLand && loose == QtNodes::PortType::Out
                           ? landing : cgo.endPoint(QtNodes::PortType::Out);

        const QPointF to = willLand && loose == QtNodes::PortType::In
                         ? landing : cgo.endPoint(QtNodes::PortType::In);

        painter->setPen(QPen(willLand ? palette::ready() : palette::caption(),
                             metrics::wireWidth, Qt::DashLine));
        painter->drawPath(wirePath(from, to));
        return;
    }

    painter->setPen(QPen(colourOf(cgo), metrics::wireWidth));
    painter->drawPath(curve);
}

QPainterPath ConnectionPainter::getPainterStroke(
    const QtNodes::ConnectionGraphicsObject& cgo) const
{
    const QPainterPath curve = wirePath(cgo);

    QPainterPath walked(cgo.endPoint(QtNodes::PortType::Out));

    for (int segment = 1; segment <= kStrokeSegments; ++segment)
    {
        walked.lineTo(curve.pointAtPercent(double(segment) / kStrokeSegments));
    }

    QPainterPathStroker stroker;
    stroker.setWidth(kStrokeWidth);

    return stroker.createStroke(walked);
}

bool ConnectionPainter::blamed(const QtNodes::ConnectionGraphicsObject& cgo) const
{
    QtNodes::AbstractGraphModel& model = const_cast<QtNodes::AbstractGraphModel&>(cgo.graphModel());
    const QtNodes::ConnectionId wire = cgo.connectionId();

    const NodeAdaptor* from = adaptorFor(model, wire.outNodeId);
    const NodeAdaptor* to   = adaptorFor(model, wire.inNodeId);

    if (from != nullptr &&
        faults.marks(static_cast<loom::NodeId>(wire.outNodeId),
                     from->pinName(QtNodes::PortType::Out, wire.outPortIndex)))
    {
        return true;
    }

    return to != nullptr &&
           faults.marks(static_cast<loom::NodeId>(wire.inNodeId),
                        to->pinName(QtNodes::PortType::In, wire.inPortIndex));
}
