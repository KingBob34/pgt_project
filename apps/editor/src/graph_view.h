#ifndef LOOM_EDITOR_GRAPH_VIEW_H
#define LOOM_EDITOR_GRAPH_VIEW_H
#include <QPoint>

#include <QtNodes/GraphicsView>

#include "loom/graph/catalog.h"

// The canvas: right button pans, left button selects, and the entry point
// cannot be deleted.
class GraphView : public QtNodes::GraphicsView
{
    Q_OBJECT

public:
    GraphView(QtNodes::BasicGraphicsScene* scene, const loom::NodeCatalog& catalog,
              QWidget* parent = nullptr);

public Q_SLOTS:
    void onDeleteSelectedObjects() override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // Every way a pan can end goes through here, or the hand cursor is left
    // lying on the canvas.
    void endPan();

    const loom::NodeCatalog& catalog;

    bool panning = false;
    bool panned  = false;
    QPoint panOrigin;
    QPoint panLast;
};

#endif //LOOM_EDITOR_GRAPH_VIEW_H
