#ifndef LOOM_EDITOR_NODE_GEOMETRY_H
#define LOOM_EDITOR_NODE_GEOMETRY_H
#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

// Node sizes, with a floor under the width.
class NodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    using QtNodes::DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry;

    void recomputeSize(QtNodes::NodeId nodeId) const override;
};

#endif //LOOM_EDITOR_NODE_GEOMETRY_H
