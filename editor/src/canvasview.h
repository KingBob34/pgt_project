#ifndef CANVASVIEW_H
#define CANVASVIEW_H
#include <QGraphicsView>
#include <QPoint>

class CanvasView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double zoom = 1.0;
    bool panning = false;
    QPoint lastPanPoint;
    void drawGrid(QPainter* painter, const QRectF& rect, double spacing);

};

#endif //CANVASVIEW_H
