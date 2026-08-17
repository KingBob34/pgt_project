#include "graph_view.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsProxyWidget>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QWheelEvent>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/Definitions>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "rubber_band_style.h"

namespace
{
    // What one turn of the wheel reports, in eighths of a degree.
    constexpr int kWheelNotch = 120;

    // The paragraph editor a point on a node lands on, if it lands on one.
    QPlainTextEdit* paragraphAt(QGraphicsItem* item, QPointF scenePos)
    {
        QGraphicsProxyWidget* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

        if (proxy == nullptr || proxy->widget() == nullptr) return nullptr;

        QWidget* under = proxy->widget()->childAt(proxy->mapFromScene(scenePos).toPoint());

        // The pointer lands on a viewport, so the editor is looked for above it.
        for (; under != nullptr; under = under->parentWidget())
        {
            if (QPlainTextEdit* box = qobject_cast<QPlainTextEdit*>(under)) return box;
        }

        return nullptr;
    }
}

GraphView::GraphView(QtNodes::BasicGraphicsScene* scene, const loom::NodeCatalog& nodeCatalog,
                     QWidget* parent)
    : QtNodes::GraphicsView(scene, parent)
    , catalog(nodeCatalog)
{
    setDragMode(QGraphicsView::RubberBandDrag);

    // The grid is drawn from the exposed rectangle, so it needs whole repaints.
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    RubberBandStyle* style = new RubberBandStyle;
    style->setParent(viewport());

    viewport()->setStyle(style);
}

void GraphView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        panning   = true;
        panned    = false;
        panOrigin = event->pos();
        panLast   = event->pos();

        viewport()->setCursor(Qt::ClosedHandCursor);

        event->accept();
        return;
    }

    QtNodes::GraphicsView::mousePressEvent(event);
}

void GraphView::mouseMoveEvent(QMouseEvent* event)
{
    if (panning)
    {
        const QPoint step = event->pos() - panLast;
        panLast = event->pos();

        // Scrolling rather than setSceneRect, which makes the view re-centre itself.
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - step.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - step.y());

        if ((event->pos() - panOrigin).manhattanLength() > QApplication::startDragDistance())
        {
            panned = true;
        }

        event->accept();
        return;
    }

    // Skips QtNodes, whose handler pans on a left drag.
    QGraphicsView::mouseMoveEvent(event);
}

void GraphView::endPan()
{
    panning = false;

    viewport()->unsetCursor();
}

void GraphView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        const bool wasPanning = panning;

        endPan();

        if (wasPanning)
        {
            event->accept();
            return;
        }
    }

    QtNodes::GraphicsView::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton) commitNodePositions();
}

void GraphView::commitNodePositions()
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return;

    QtNodes::AbstractGraphModel& model = graph->graphModel();

    for (QGraphicsItem* item : graph->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        const QPointF drawn = object->pos();
        const QPointF stored =
            model.nodeData(object->nodeId(), QtNodes::NodeRole::Position).value<QPointF>();

        if (drawn != stored)
        {
            model.setNodeData(object->nodeId(), QtNodes::NodeRole::Position, drawn);
        }
    }
}

void GraphView::keyReleaseEvent(QKeyEvent* event)
{
    QtNodes::GraphicsView::keyReleaseEvent(event);

    // The base class answers a Shift release by going back to panning.
    setDragMode(QGraphicsView::RubberBandDrag);
}

void GraphView::contextMenuEvent(QContextMenuEvent* event)
{
    // The menu takes the mouse, so the release that would end the pan never
    // arrives here.
    const bool dragged = panned;

    endPan();

    // A dragged right button was a pan, not a request for the menu.
    if (dragged)
    {
        event->accept();
        return;
    }

    QtNodes::GraphicsView::contextMenuEvent(event);
}

void GraphView::wheelEvent(QWheelEvent* event)
{
    const QPoint at = event->position().toPoint();

    // A text box under the pointer takes the whole wheel, including the turns
    // that scroll nothing: reaching the last line must not start a zoom.
    if (QPlainTextEdit* box = paragraphAt(itemAt(at), mapToScene(at)))
    {
        QScrollBar* bar = box->verticalScrollBar();
        const int lines = event->angleDelta().y() * QApplication::wheelScrollLines() / kWheelNotch;

        bar->setValue(bar->value() - lines * bar->singleStep());

        event->accept();
        return;
    }

    QtNodes::GraphicsView::wheelEvent(event);
}

void GraphView::onDeleteSelectedObjects()
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();

    if (graph != nullptr)
    {
        // The entry point leaves the selection before the deletion runs.
        for (QGraphicsItem* item : graph->selectedItems())
        {
            QtNodes::NodeGraphicsObject* object =
                qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

            if (object == nullptr) continue;

            const QString name = graph->graphModel().nodeData(object->nodeId(),
                                                              QtNodes::NodeRole::Type).toString();

            const loom::NodeType* type = catalog.find(name.toStdString());

            if (type != nullptr && type->isEntryPoint()) object->setSelected(false);
        }
    }

    QtNodes::GraphicsView::onDeleteSelectedObjects();
}
