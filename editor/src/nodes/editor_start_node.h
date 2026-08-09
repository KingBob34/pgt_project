#ifndef EDITOR_START_NODE_H
#define EDITOR_START_NODE_H
#include "nodes/editor_node.h"

// Where execution begins. Exactly one per graph
class StartNode : public EditorNode
{
    Q_OBJECT
public:
    QString name() const override
    {
        return "start";
    }
    QString caption() const override
    {
        return "Start";
    }
    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::Out ? 1 : 0;
    }
    QString portCaption(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return "out";
    }
    StartNode()
    {
        setHeaderColor(QColor(150, 35, 35));
    }
};

#endif //EDITOR_START_NODE_H
