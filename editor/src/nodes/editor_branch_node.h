#ifndef EDITOR_BRANCH_NODE_H
#define EDITOR_BRANCH_NODE_H
#include "nodes/editor_node.h"

// Branches on a bool taken from a data pin.
class BranchNode : public EditorNode
{
    Q_OBJECT
public:
    BranchNode()
    {
        setHeaderColor(QColor(55, 75, 105));
    }

    QString name() const override { return "branch"; }
    QString caption() const override { return "Branch"; }

    unsigned int nPorts(QtNodes::PortType) const override
    {
        return 2;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType,
                                   QtNodes::PortIndex portIndex) const override
    {
        // Input 1 is the condition; every other pin is execution flow
        if (portType == QtNodes::PortType::In && portIndex == 1) return boolPin();
        return flowPin();
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return portIndex == 0 ? "in" : "condition";
        }
        return portIndex == 0 ? "true" : "false";
    }
};

#endif //EDITOR_BRANCH_NODE_H
