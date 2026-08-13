#ifndef LOOM_EDITOR_NODE_GEOMETRY_H
#define LOOM_EDITOR_NODE_GEOMETRY_H
#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

class NodeAdaptor;

// The vertical distance from one port to the next, and one editor row's height.
int portRowHeight();

// Node sizes, with a floor under the width.
class NodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    using QtNodes::DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry;

    void recomputeSize(QtNodes::NodeId nodeId) const override;

    QPointF portPosition(QtNodes::NodeId nodeId, QtNodes::PortType portType,
                         QtNodes::PortIndex index) const override;

private:
    const NodeAdaptor* adaptorFor(QtNodes::NodeId nodeId) const;
};

#endif //LOOM_EDITOR_NODE_GEOMETRY_H
