#ifndef EDITOR_CHOICE_NODE_H
#define EDITOR_CHOICE_NODE_H
#include "nodes/editor_node.h"

// Offers options to the player and waits.
class ChoiceNode : public EditorNode
{
    Q_OBJECT
public:
    [[nodiscard]] QString name() const override
    {
        return "choice";
    }
    [[nodiscard]] QString caption() const override
    {
        return "Choice";
    }
    [[nodiscard]] unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? 1 : 2;
    }
    [[nodiscard]] QString portCaption(QtNodes::PortType portType,
                                      QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) return "in";
        return "option " + QString::number(portIndex + 1);
    }
};

#endif //EDITOR_CHOICE_NODE_H
