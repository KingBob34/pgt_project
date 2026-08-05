#include "canvasview.h"
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPainter>
#include <QLineF>
#include <cmath>

CanvasView::CanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    QGraphicsScene* scene = new QGraphicsScene(this);
    scene->setSceneRect(-5000, -5000, 10000, 10000);
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
}

void CanvasView::wheelEvent(QWheelEvent* event)
{
    const double step = 1.15;
    double factor = (event->angleDelta().y() > 0) ? step : 1.0 / step;
    double newZoom = zoom * factor;
    if (newZoom < 0.2 || newZoom > 5.0)
    {
        return;
    }
    zoom = newZoom;
    scale(factor, factor);
}

void CanvasView::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, QColor(32, 33, 38));
    const double fine = 25.0;
    const double coarse = fine * 5.0;
    // Fine grid: only when zoomed in enough (level of detail)
    if (zoom > 0.5) {
        QPen finePen(QColor(45, 46, 52));
        finePen.setCosmetic(true);
        painter->setPen(finePen);
        drawGrid(painter, rect, fine);
    }
    // Coarse grid: always visible
    QPen coarsePen(QColor(60, 61, 70));
    coarsePen.setCosmetic(true);
    painter->setPen(coarsePen);
    drawGrid(painter, rect, coarse);
}

void CanvasView::drawGrid(QPainter* painter, const QRectF& rect, double spacing)
{
    double left = std::floor(rect.left() / spacing) * spacing;
    double top  = std::floor(rect.top()  / spacing) * spacing;

    for (double x = left; x < rect.right(); x += spacing) {
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (double y = top; y < rect.bottom(); y += spacing) {
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
}

void CanvasView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        panning = true;
        lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event)
{
    if (panning) {
        QPoint delta = event->pos() - lastPanPoint;
        lastPanPoint = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && panning) {
        panning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
