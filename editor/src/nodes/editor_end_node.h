#ifndef EDITOR_END_NODE_H
#define EDITOR_END_NODE_H
#include "nodes/editor_node.h"

// Terminates the story.
class EndNode : public EditorNode
{
    Q_OBJECT
public:
    [[nodiscard]] QString name() const override
    {
        return "end";
    }
    [[nodiscard]] QString caption() const override
    {
        return "End";
    }
    [[nodiscard]] unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? 1 : 0;
    }
    [[nodiscard]] QString portCaption(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return "in";
    }
    EndNode()
    {
        setHeaderColor(QColor(150, 35, 35));
    }
};

#endif //EDITOR_END_NODE_H
