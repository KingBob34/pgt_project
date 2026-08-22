#include "node_painter.h"

#include <cmath>

#include <QFont>
#include <QFontMetricsF>
#include <QPainterPath>
#include <QPolygonF>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/BasicGraphicsScene.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/NodeState.hpp>

#include "loom/graph/pin.h"

#include "canvas_faults.h"
#include "graph_scene.h"
#include "connection_painter.h"
#include "node_adaptor.h"
#include "node_metrics.h"
#include "node_palette.h"

namespace
{
    constexpr double kRadius = 5.0;
    constexpr double kBorderWidth = 1.0;
    constexpr double kSelectedWidth = 2.0;
    constexpr double kFaultWidth = 2.5;

    // The band the geometry keeps clear at the top for the caption.
    double titleHeight(QtNodes::NodeGraphicsObject& ngo)
    {
        return ngo.nodeScene()->nodeGeometry().captionRect(ngo.nodeId()).height()
               + metrics::portSpacing;
    }

    QRectF outline(QtNodes::NodeGraphicsObject& ngo)
    {
        const QSize size = ngo.nodeScene()->nodeGeometry().size(ngo.nodeId());

        return QRectF(0.0, 0.0, size.width(), size.height());
    }

    // The wire is behind the card here, so it is shown through it.
    constexpr int kTailAlpha = 120;

    // A frame is a pane the author reads the graph through.
    constexpr int kFrameAlpha = 150;
    constexpr double kFrameTitleGap = 10.0;

    // The stretch of a wire a card is standing on: walk out from the port until
    // the curve leaves the card, and stop. Drawing the whole curve would also
    // light up any later part of it that happens to cross this same node.
    QPainterPath tailInside(const QPainterPath& curve, const QRectF& card, bool fromStart)
    {
        constexpr int kSteps = 200;

        QPainterPath tail(curve.pointAtPercent(fromStart ? 0.0 : 1.0));

        for (int step = 1; step <= kSteps; ++step)
        {
            const double along = double(step) / kSteps;
            const QPointF at = curve.pointAtPercent(fromStart ? along : 1.0 - along);

            tail.lineTo(at);

            if (!card.contains(at)) break;
        }

        return tail;
    }

    constexpr double kPortRadius = 4.5;
    constexpr double kPortWidth = 1.6;

    // The flow connector is the tall one: it is what the author's eye follows
    // across a graph, and every node has at most two of them.
    constexpr double kFlowHalfWidth = 5.0;
    constexpr double kFlowHalfHeight = 6.5;

    // The mark that says which way a value travels, small enough to read as
    // part of the connector rather than as a second one.
    constexpr double kBeakLength = 4.0;
    constexpr double kBeakHalfHeight = 3.0;
    constexpr double kBeakGap = 2.0;

    // While a wire is being dragged, a port it could land on swells as the wire
    // nears and one it could not shrinks away. Kept from the base class, whose
    // port drawing this replaces.
    double reaction(QtNodes::NodeGraphicsObject& ngo, QtNodes::PortType side,
                    QtNodes::PortIndex index, const QPointF& at)
    {
        const QtNodes::ConnectionGraphicsObject* dragged =
            ngo.nodeState().connectionForReaction();

        if (dragged == nullptr) return 1.0;

        const QtNodes::PortType wanted = dragged->connectionState().requiredPort();
        if (wanted != side) return 1.0;

        const bool possible = ngo.graphModel().connectionPossible(
            QtNodes::makeCompleteConnectionId(dragged->connectionId(), ngo.nodeId(), index));

        QPointF end = dragged->sceneTransform().map(dragged->endPoint(wanted));
        end = ngo.sceneTransform().inverted().map(end);

        const QPointF away = end - at;
        const double distance = std::sqrt(QPointF::dotProduct(away, away));

        if (possible)
        {
            constexpr double kNear = 40.0;

            return distance < kNear ? 2.0 - distance / kNear : 1.0;
        }

        constexpr double kFar = 80.0;

        return distance < kFar ? distance / kFar : 1.0;
    }

    // A flat back and a point, the shape Blueprint gives an exec pin. Flow runs
    // left to right, so the point does too.
    void drawFlow(QPainter* painter, const QPointF& at, double scale)
    {
        const double half = kFlowHalfWidth * scale;
        const double rise = kFlowHalfHeight * scale;

        QPolygonF head;
        head << QPointF(at.x() - half, at.y() - rise)
             << QPointF(at.x(), at.y() - rise)
             << QPointF(at.x() + half, at.y())
             << QPointF(at.x(), at.y() + rise)
             << QPointF(at.x() - half, at.y() + rise);

        painter->drawPolygon(head);
    }

    // Says which way the value travels, and points that way on both sides of a
    // node, because a value leaving one node is the same value arriving at the
    // next. Which edge the connector sits on is what says input or output.
    void drawBeak(QPainter* painter, const QPointF& at, double scale)
    {
        const double radius = kPortRadius * scale;
        const double rise = kBeakHalfHeight * scale;
        const double length = kBeakLength * scale;

        const double back = at.x() + radius + kBeakGap;

        QPolygonF beak;
        beak << QPointF(back, at.y() - rise)
             << QPointF(back + length, at.y())
             << QPointF(back, at.y() + rise);

        painter->drawPolygon(beak);
    }

    void drawPort(QPainter* painter, const QPointF& at, const std::string& type, bool wired,
                  double scale)
    {
        const QColor tint = palette::pin(type);
        const double radius = kPortRadius * scale;

        painter->setPen(QPen(tint, kPortWidth));
        painter->setBrush(wired ? QBrush(tint) : QBrush(palette::body()));

        // Never swells or shrinks: the arrow is the largest connector on the
        // card, and a large shape changing size reads as a fault rather than
        // as an answer to the wire being offered.
        if (type == loom::PinType::Flow)
        {
            drawFlow(painter, at, 1.0);
            return;
        }

        // A list holds many of something, and a square says so at a size where
        // anything drawn inside a circle would be mush.
        if (type == loom::PinType::List)
        {
            painter->drawRect(QRectF(at.x() - radius, at.y() - radius, radius * 2.0, radius * 2.0));
        }
        else
        {
            painter->drawEllipse(at, radius, radius);
        }

        // Never scaled: the swell is the answer to a wire being offered, and
        // the mark that says which way the value travels is not part of that.
        painter->setPen(Qt::NoPen);
        painter->setBrush(tint);
        drawBeak(painter, at, 1.0);
    }

}

NodePainter::NodePainter(const CanvasFaults& canvasFaults)
    : faults(canvasFaults)
{
}

void NodePainter::paint(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    if (const NodeAdaptor* adaptor = adaptorFor(ngo.graphModel(), ngo.nodeId());
        adaptor != nullptr && adaptor->isFrame())
    {
        drawFrame(painter, ngo, *adaptor);
        return;
    }

    drawCard(painter, ngo);
    drawTitle(painter, ngo);
    drawWireTails(painter, ngo);
    drawBorder(painter, ngo);

    drawPorts(painter, ngo);
    drawEntryLabels(painter, ngo);

    if (const NodeAdaptor* adaptor = adaptorFor(ngo.graphModel(), ngo.nodeId());
        adaptor != nullptr && adaptor->isResizable())
    {
        drawGrip(painter, outline(ngo));
    }
}

void NodePainter::drawFrame(QPainter* painter, QtNodes::NodeGraphicsObject& ngo,
                            const NodeAdaptor& adaptor) const
{
    const QRectF pane = outline(ngo);

    painter->setRenderHint(QPainter::Antialiasing);

    // The pane the author reads the graph through.
    QColor glass = palette::body();
    glass.setAlpha(kFrameAlpha);

    painter->setBrush(glass);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(pane, kRadius, kRadius);

    QFont bold = painter->font();
    bold.setBold(true);
    bold.setPointSize(bold.pointSize() + 2);

    const QFontMetricsF metrics(bold);
    const double band = metrics.height() + 2.0 * kFrameTitleGap;

    // A strip in its category's colour across the top, the way a node wears
    // one, and solid so the title is read against it rather than through it.
    QPainterPath card;
    card.addRoundedRect(pane, kRadius, kRadius);

    QPainterPath top;
    top.addRect(QRectF(pane.left(), pane.top(), pane.width(), band));

    painter->fillPath(card.intersected(top), palette::title(adaptor.nodeType()));

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(palette::border(ngo.isSelected()), ngo.isSelected() ? 2.0 : 1.0));
    painter->drawRoundedRect(pane.adjusted(1.0, 1.0, -1.0, -1.0), kRadius, kRadius);

    // Left, not centred: a frame is wide, and a title in the middle of one
    // reads as a label for whatever happens to be under it.
    const GraphScene* graph = qobject_cast<GraphScene*>(ngo.nodeScene());
    const bool retyping = graph != nullptr && graph->renaming() == ngo.nodeId();

    if (!retyping)
    {
        painter->setFont(bold);
        painter->setPen(palette::caption());
        painter->drawText(QPointF(kFrameTitleGap,
                                  (band + metrics.ascent() - metrics.descent()) / 2.0),
                          adaptor.caption());
    }

    drawGrip(painter, pane);
}

// Drawn rather than left to be found by feel.
void NodePainter::drawGrip(QPainter* painter, const QRectF& pane) const
{
    painter->setPen(QPen(palette::caption(), 1.5));

    for (int step = 1; step <= 3; ++step)
    {
        const double along = step * 4.0;

        painter->drawLine(QPointF(pane.right() - along, pane.bottom() - 3.0),
                          QPointF(pane.right() - 3.0, pane.bottom() - along));
    }
}

void NodePainter::drawCard(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(palette::body());
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(outline(ngo), kRadius, kRadius);
}

void NodePainter::drawBorder(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    const bool blamed = faults.marks(static_cast<loom::NodeId>(ngo.nodeId()));

    // A fault outranks the selection: the author needs to see it whether or
    // not the node happens to be the one they are holding.
    double width = ngo.isSelected() ? kSelectedWidth : kBorderWidth;
    QColor edge = palette::border(ngo.isSelected());

    if (blamed)
    {
        width = kFaultWidth;
        edge = palette::fault();
    }

    if (ngo.nodeState().hovered() && !blamed) width += 1.0;

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(edge, width));

    // Inset by half the pen, or the stroke is drawn half outside the card.
    painter->drawRoundedRect(
        outline(ngo).adjusted(width / 2.0, width / 2.0, -width / 2.0, -width / 2.0),
        kRadius, kRadius);
}

void NodePainter::drawTitle(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    const NodeAdaptor* adaptor = adaptorFor(ngo.graphModel(), ngo.nodeId());
    if (adaptor == nullptr) return;

    const QRectF body = outline(ngo);
    const double height = titleHeight(ngo);

    // The strip is the top of the card, so it takes the card's rounded corners
    // and leaves the square ones where it meets the rows.
    QPainterPath card;
    card.addRoundedRect(body, kRadius, kRadius);

    QPainterPath band;
    band.addRect(QRectF(body.left(), body.top(), body.width(), height));

    painter->setPen(Qt::NoPen);
    painter->fillPath(card.intersected(band), palette::title(adaptor->nodeType()));

    QFont bold = painter->font();
    bold.setBold(true);

    // Centred in the strip by the font's own ascent and descent. The geometry's
    // caption position sits low enough for descenders to touch the edge.
    const QFontMetricsF metrics(bold);
    const double baseline = (height + metrics.ascent() - metrics.descent()) / 2.0;
    const QString caption = adaptor->caption();

    painter->setFont(bold);
    painter->setPen(palette::caption());
    painter->drawText(QPointF((body.width() - metrics.horizontalAdvance(caption)) / 2.0, baseline),
                      caption);

    bold.setBold(false);
    painter->setFont(bold);
}

void NodePainter::drawPorts(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    QtNodes::AbstractGraphModel&   model = ngo.graphModel();
    QtNodes::AbstractNodeGeometry& geometry = ngo.nodeScene()->nodeGeometry();

    const QtNodes::NodeId node = ngo.nodeId();

    painter->setRenderHint(QPainter::Antialiasing);

    for (QtNodes::PortType side : { QtNodes::PortType::Out, QtNodes::PortType::In })
    {
        const QtNodes::NodeRole counted = side == QtNodes::PortType::Out
                                        ? QtNodes::NodeRole::OutPortCount
                                        : QtNodes::NodeRole::InPortCount;

        const unsigned int ports = model.nodeData(node, counted).toUInt();

        for (unsigned int port = 0; port < ports; ++port)
        {
            const QtNodes::PortIndex index = static_cast<QtNodes::PortIndex>(port);
            const QPointF at = geometry.portPosition(node, side, index);

            const QtNodes::NodeDataType carried =
                model.portData(node, side, index, QtNodes::PortRole::DataType)
                     .value<QtNodes::NodeDataType>();

            drawPort(painter, at, carried.id.toStdString(),
                     !model.connections(node, side, index).empty(),
                     reaction(ngo, side, index, at));
        }
    }

    // The reaction lasts one repaint, and clearing it is what ends it.
    if (ngo.nodeState().connectionForReaction()) ngo.nodeState().resetConnectionForReaction();
}

void NodePainter::drawWireTails(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const
{
    QtNodes::AbstractGraphModel& model = ngo.graphModel();
    QtNodes::BasicGraphicsScene* graph = ngo.nodeScene();

    const QtNodes::NodeId node = ngo.nodeId();
    const QRectF card = outline(ngo);
    const QTransform fromScene = ngo.sceneTransform().inverted();

    painter->save();
    painter->setClipRect(card);
    painter->setBrush(Qt::NoBrush);

    for (QtNodes::PortType side : { QtNodes::PortType::Out, QtNodes::PortType::In })
    {
        const QtNodes::NodeRole counted = side == QtNodes::PortType::Out
                                        ? QtNodes::NodeRole::OutPortCount
                                        : QtNodes::NodeRole::InPortCount;

        const unsigned int ports = model.nodeData(node, counted).toUInt();

        for (unsigned int port = 0; port < ports; ++port)
        {
            const QtNodes::PortIndex index = static_cast<QtNodes::PortIndex>(port);

            const QtNodes::NodeDataType carried =
                model.portData(node, side, index, QtNodes::PortRole::DataType)
                     .value<QtNodes::NodeDataType>();

            // Dimmer than the wire outside, because from here it is behind the
            // card rather than on the canvas.
            QColor under = palette::pin(carried.id.toStdString());
            under.setAlpha(kTailAlpha);

            painter->setPen(QPen(under, metrics::wireWidth));

            for (const QtNodes::ConnectionId& wire : model.connections(node, side, index))
            {
                QtNodes::ConnectionGraphicsObject* drawn = graph->connectionGraphicsObject(wire);
                if (drawn == nullptr) continue;

                const QPainterPath curve =
                    (drawn->sceneTransform() * fromScene).map(wirePath(*drawn));

                painter->drawPath(tailInside(curve, card, side == QtNodes::PortType::Out));
            }
        }
    }

    painter->restore();
}
