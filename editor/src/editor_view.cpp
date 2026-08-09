#include "editor_view.h"
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QProxyStyle>
#include <QScrollBar>
#include <QStyleOption>
#include <QKeyEvent>
#include <cmath>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

namespace
{
    // Selection rectangle drawn as marching ants
    class RubberBandStyle : public QProxyStyle
    {
    public:
        void drawControl(ControlElement element,
            const QStyleOption* opt, QPainter* p, const QWidget* w) const override
        {
            if (element != CE_RubberBand)
            {
                QProxyStyle::drawControl(element, opt, p, w);
                return;
            }
            p->save();
            p->setRenderHint(QPainter::Antialiasing, false);
            p->setBrush(Qt::NoBrush);

            // Half pixel inset so a 1 pixel pen lands on a pixel
            const QRectF rect = QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5);
            p->setPen(QPen(Qt::black, 1));
            p->drawRect(rect);

            constexpr qreal period = 8.0;
            QPen white(Qt::white, 1);
            white.setDashPattern({4, 4});

            white.setDashOffset(std::fmod(rect.left(), period));
            p->setPen(white);
            p->drawLine(rect.topLeft(), rect.topRight());
            p->drawLine(rect.bottomLeft(), rect.bottomRight());
            white.setDashOffset(std::fmod(rect.top(), period));
            p->setPen(white);
            p->drawLine(rect.topLeft(), rect.bottomLeft());
            p->drawLine(rect.topRight(), rect.bottomRight());
            p->restore();
        }
    };
}

EditorView::EditorView(QtNodes::BasicGraphicsScene* scene, QWidget* parent)
    : QtNodes::GraphicsView(scene, parent)
{
    // Left drag on empty canvas draws a selection rectangle
    setDragMode(QGraphicsView::RubberBandDrag);

    // The grid is drawn in drawBackground() and needs full repaints
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheNone);

    auto* rubberBandStyle = new RubberBandStyle;
    rubberBandStyle->setParent(viewport());
    viewport()->setStyle(rubberBandStyle);
}

void EditorView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        panning = true;
        panMoved = false;
        panStartPosition = event->pos();
        lastPanPosition = event->pos();
        return;
    }
    QtNodes::GraphicsView::mousePressEvent(event);
}

void EditorView::mouseMoveEvent(QMouseEvent* event)
{
    if (panning)
    {
        const QPoint delta = event->pos() - lastPanPosition;
        lastPanPosition = event->pos();
        if ((event->pos() - panStartPosition).manhattanLength() > kClickSlack)
        {
            panMoved = true;
        }
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        return;
    }
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    QGraphicsView::mouseMoveEvent(event);
}

void EditorView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && panning)
    {
        panning = false;
        return;
    }
    QtNodes::GraphicsView::mouseReleaseEvent(event);
}

void EditorView::contextMenuEvent(QContextMenuEvent* event)
{
    if (panMoved)
    {
        panMoved = false;
        return;
    }
    QtNodes::GraphicsView::contextMenuEvent(event);
}

void EditorView::protectFromDeletion(QtNodes::NodeId const nodeId)
{
    undeletableNodes.insert(nodeId);
}

void EditorView::onDeleteSelectedObjects()
{
    // Protected nodes are deselected first; the base deletes the rest
    if (auto* graphScene = qobject_cast<QtNodes::BasicGraphicsScene*>(scene()))
    {
        for (const QtNodes::NodeId nodeId : undeletableNodes)
        {
            if (auto* object = graphScene->nodeGraphicsObject(nodeId))
            {
                object->setSelected(false);
            }
        }
    }

    QtNodes::GraphicsView::onDeleteSelectedObjects();
}

void EditorView::keyReleaseEvent(QKeyEvent* event)
{
    QtNodes::GraphicsView::keyReleaseEvent(event);

    // Releasing Shift resets the drag mode in the base class
    if (dragMode() != QGraphicsView::RubberBandDrag)
    {
        setDragMode(QGraphicsView::RubberBandDrag);
    }
}
