#ifndef LOOM_EDITOR_NODE_GEOMETRY_H
#define LOOM_EDITOR_NODE_GEOMETRY_H
#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

// The vertical distance from one port to the next, which is also the height an
// embedded editor takes so that it lines up with the port it belongs to.
int portRowHeight();

// Node sizes, with a floor under the width.
class NodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    using QtNodes::DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry;

    void recomputeSize(QtNodes::NodeId nodeId) const override;
};

#endif //LOOM_EDITOR_NODE_GEOMETRY_H
