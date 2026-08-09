#ifndef EDITOR_NODE_GEOMETRY_H
#define EDITOR_NODE_GEOMETRY_H
#include <QtNodes/internal/DefaultHorizontalNodeGeometry.hpp>

// Keeps nodes readably wide.
class EditorNodeGeometry : public QtNodes::DefaultHorizontalNodeGeometry
{
public:
    using DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry;

    void recomputeSize(QtNodes::NodeId nodeId) const override;
};

#endif //EDITOR_NODE_GEOMETRY_H
