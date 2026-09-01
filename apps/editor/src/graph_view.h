#ifndef LOOM_EDITOR_GRAPH_VIEW_H
#define LOOM_EDITOR_GRAPH_VIEW_H
#include <vector>

#include <QByteArray>
#include <QPoint>
#include <QSize>

#include <QtNodes/GraphicsView>

#include "loom/graph/catalog.h"

namespace QtNodes
{
    class NodeGraphicsObject;
}

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

    // A scene comes with its entry point and may have only the one, so it is
    // left out of everything that would make a second.
    void onCopySelectedObjects() override;
    void onDuplicateSelectedObjects() override;
    void onPasteObjects() override;

    // Copy, then delete what was copied.
    void onCutSelectedObjects();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Every way a pan can end goes through here, or the hand cursor is left
    // lying on the canvas.
    void endPan();

    bool isEntryPoint(QtNodes::NodeId node);

    // The same document with every entry point, and every wire that reached
    // one, left out. Handed back unchanged when there were none.
    QByteArray withoutEntryPoints(const QByteArray& document) const;

    // Takes every entry point out of the selection and hands them back, so the
    // caller can put the author's selection the way it found it.
    std::vector<QtNodes::NodeGraphicsObject*> dropEntryPoints();

    // Whether a pin's editor has the keyboard, in which case the canvas has
    // no business reading the keys it is being sent.
    bool typing() const;

    // The node menu, raised where a wire was let go of. The draft names that
    // wire, so the menu can offer only what would take it.
    void openNodeMenu(const QPoint& at, const QtNodes::ConnectionId& draft);

    // The frame whose title is written under this point, and the box that
    // edits that title where it stands.
    bool frameTitleAt(const QPointF& scenePos, QtNodes::NodeId& frame);
    void renameFrame(QtNodes::NodeId frame);

    // Cuts every wire reaching the pin under this point. False when the point
    // is not on a pin at all, so the press can go on to mean what it usually
    // means.
    bool breakWiresAt(const QPointF& scenePos);

    // The frame whose corner grip is under this point, if one is.
    class NodeAdaptor* gripAt(const QPointF& scenePos, QtNodes::NodeId& frame);

    // Everything a frame has completely surrounded joins it in the selection,
    // so that dragging the frame drags what the author drew it round.
    void gatherFramed(QtNodes::NodeId frame);

    // QtNodes moves the drawn node and never tells the model, so a dragged
    // node is saved where it used to be. Reads the truth back at every drop.
    void commitNodePositions();

    const loom::NodeCatalog& catalog;

    // A wire is being dragged, and whether it found a port before it was
    // dropped. A wire let go of over nothing offers to make a node instead.
    bool drafting = false;
    bool landed = false;

    // A frame is being resized by its corner.
    QtNodes::NodeId sizing = QtNodes::InvalidNodeId;
    QPoint          sizingFrom;
    QSize           sizingWas;

    bool panning = false;
    bool panned  = false;
    QPoint panOrigin;
    QPoint panLast;

    // Where the canvas's top left corner sits in the window, so that a panel
    // moving that corner can be answered by scrolling as far the other way.
    QPoint corner;
};

#endif //LOOM_EDITOR_GRAPH_VIEW_H
