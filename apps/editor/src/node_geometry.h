#ifndef LOOM_EDITOR_NODE_GEOMETRY_H
#define LOOM_EDITOR_NODE_GEOMETRY_H
#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

// Node sizes, with a floor under the width, ports that follow the height of
// the editor row beside them rather than being evenly spaced, and connectors
// set inside the card instead of on its edge.
class NodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    using QtNodes::DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry;

    void recomputeSize(QtNodes::NodeId nodeId) const override;

    // The base class leaves twenty pixels of air around every node, which the
    // rubber band would then have to cover before the node counted as inside.
    QRectF boundingRect(QtNodes::NodeId nodeId) const override;

    QPointF portPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                         QtNodes::PortIndex index) const override;

    // Both move inwards with the ports, or the connectors land on the captions.
    QPointF portTextPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                             QtNodes::PortIndex index) const override;

    QPointF widgetPosition(QtNodes::NodeId nodeId) const override;

private:
    // How far in from one edge that side's connectors sit. Nothing on a side
    // with no connectors, which is what keeps a value node from carrying the
    // width of an input column it does not have.
    int inset(QtNodes::NodeId nodeId, QtNodes::PortType portType) const;
};

#endif //LOOM_EDITOR_NODE_GEOMETRY_H
