#ifndef EDITOR_NARRATIVE_NODE_H
#define EDITOR_NARRATIVE_NODE_H
#include "nodes/editor_node.h"

// Shows a line of text, then carries on.
class NarrativeNode : public EditorNode
{
    Q_OBJECT
public:
    [[nodiscard]] QString name() const override
    {
        return "narrative";
    }
    [[nodiscard]] QString caption() const override
    {
        return "Narrative";
    }
    [[nodiscard]] unsigned int nPorts(QtNodes::PortType) const override
    {
        return 1;
    }
    [[nodiscard]] QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex) const override
    {
        return portType == QtNodes::PortType::In ? "in" : "out";
    }
};

#endif //EDITOR_NARRATIVE_NODE_H
