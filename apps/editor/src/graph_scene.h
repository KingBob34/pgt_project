#ifndef LOOM_EDITOR_GRAPH_SCENE_H
#define LOOM_EDITOR_GRAPH_SCENE_H
#include <QtNodes/DataFlowGraphicsScene>

#include "loom/graph/catalog.h"

// A draft wire with neither end on a node, which is what the node menu is
// handed when it was asked for on its own rather than by a wire.
inline constexpr QtNodes::ConnectionId kNoWire{ QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex,
                                                QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex };

class GraphScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT

public:
    GraphScene(QtNodes::DataFlowGraphModel& model, const loom::NodeCatalog& catalog,
               QObject* parent = nullptr);

    QMenu* createSceneMenu(QPointF scenePos) override;

    // The same menu, narrowed to the nodes that could take the wire the author
    // let go of over empty canvas, and joined to it once one is chosen.
    QMenu* createWireMenu(QPointF scenePos, const QtNodes::ConnectionId& draft);

public:
    // Two things QtNodes leaves off a node that a card drawn like ours needs,
    // and the layer a frame belongs on. Run again whenever the canvas has had
    // a chance to reorder itself.
    void prepareNodes();

    // QtNodes gives a node's widget a proxy when the node is made and never
    // resizes it again -- it only moves it. A node whose rows have changed
    // height leaves the bottom of its widget outside that stale rectangle,
    // where it is clipped away.
    void refitWidget(QtNodes::NodeId node);

    // Handling a node raises it above the others; a frame has to go back down.
    // Cheap enough to call on every mouse move: it touches only what is wrong.
    void sinkFrames();

    // The frame whose title is being retyped. Its painter leaves the title off
    // while the box is up, or the old name shows through the new one.
    QtNodes::NodeId renaming() const { return beingNamed; }
    void setRenaming(QtNodes::NodeId frame) { beingNamed = frame; }

private:
    // The one menu behind both of the above. A draft with neither end on a
    // node is the plain menu, which offers the whole catalog.
    QMenu* buildNodeMenu(QPointF scenePos, const QtNodes::ConnectionId& draft);

    // Which side of a new node the loose end of a draft needs, and what the
    // end already on the canvas carries.
    QtNodes::PortType wantedSide(const QtNodes::ConnectionId& draft) const;
    std::string       carriedType(const QtNodes::ConnectionId& draft);

    // One step on the undo stack: the node, and the wire it was reached by.
    void addNode(const QString& type, QPointF at, const QtNodes::ConnectionId& draft,
                 const std::string& landing);

    QtNodes::NodeId beingNamed = QtNodes::InvalidNodeId;

    const loom::NodeCatalog& catalog;
};

#endif //LOOM_EDITOR_GRAPH_SCENE_H
