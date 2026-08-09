#ifndef EDITOR_COMPARE_NODE_H
#define EDITOR_COMPARE_NODE_H
#include "nodes/editor_node.h"

// Two values in, one bool out. Subclasses supply only a name and a symbol.
class CompareNode : public EditorNode
{
    Q_OBJECT
public:
    CompareNode()
    {
        setHeaderColor(QColor(45, 85, 65));
    }

    int minimumWidth() const override
    {
        return kOperatorNodeWidth;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex) const override
    {
        return portType == QtNodes::PortType::In ? valuePin() : boolPin();
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::Out) return QString();
        return portIndex == 0 ? "a" : "b";
    }

    bool portCaptionVisible(QtNodes::PortType portType, QtNodes::PortIndex) const override
    {
        return portType == QtNodes::PortType::In;
    }
};

class EqualNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "equal"; }
    QString caption() const override { return "=="; }
};

class NotEqualNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "notEqual"; }
    QString caption() const override { return "!="; }
};

class LessNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "less"; }
    QString caption() const override { return "<"; }
};

class LessEqualNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "lessEqual"; }
    QString caption() const override { return "<="; }
};

class GreaterNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "greater"; }
    QString caption() const override { return ">"; }
};

class GreaterEqualNode : public CompareNode
{
    Q_OBJECT
public:
    QString name() const override { return "greaterEqual"; }
    QString caption() const override { return ">="; }
};

// Whether a list holds an item. Different pin types, so not a CompareNode.
class ContainsNode : public EditorNode
{
    Q_OBJECT
public:
    ContainsNode()
    {
        setHeaderColor(QColor(45, 85, 65));
    }

    QString name() const override { return "contains"; }
    QString caption() const override { return "contains"; }

    int minimumWidth() const override
    {
        return kOperatorNodeWidth;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType,
                                   QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::Out) return boolPin();
        return portIndex == 0 ? listPin() : valuePin();
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::Out) return QString();
        return portIndex == 0 ? "list" : "item";
    }

    bool portCaptionVisible(QtNodes::PortType portType, QtNodes::PortIndex) const override
    {
        return portType == QtNodes::PortType::In;
    }
};

#endif //EDITOR_COMPARE_NODE_H
