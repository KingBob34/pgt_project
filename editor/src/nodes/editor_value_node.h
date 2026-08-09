#ifndef EDITOR_VALUE_NODE_H
#define EDITOR_VALUE_NODE_H
#include "nodes/editor_node.h"
#include <QLineEdit>

// Value sources. No flow pins: read on demand by whatever needs the value.
class ValueNode : public EditorNode
{
    Q_OBJECT
public:
    ValueNode()
    {
        setHeaderColor(QColor(40, 75, 95));
    }

    int minimumWidth() const override
    {
        return kOperatorNodeWidth;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::Out ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return outputType();
    }

    bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return false;
    }

    // Built on first use, never in the constructor
    QWidget* embeddedWidget() override
    {
        if (field == nullptr)
        {
            field = new QLineEdit();
            field->setPlaceholderText(placeholder());
            field->setFixedWidth(90);
        }
        return field;
    }

protected:
    [[nodiscard]] virtual QtNodes::NodeDataType outputType() const
    {
        return valuePin();
    }

    [[nodiscard]] virtual QString placeholder() const = 0;

private:
    QLineEdit* field = nullptr;
};

// Reads a variable from the game state.
class GetVarNode : public ValueNode
{
    Q_OBJECT
public:
    QString name() const override { return "getVar"; }
    QString caption() const override { return "Get"; }

protected:
    [[nodiscard]] QString placeholder() const override { return "variable"; }
};

// Reads a list variable.
class GetListNode : public ValueNode
{
    Q_OBJECT
public:
    QString name() const override { return "getList"; }
    QString caption() const override { return "Get List"; }

protected:
    [[nodiscard]] QtNodes::NodeDataType outputType() const override { return listPin(); }
    [[nodiscard]] QString placeholder() const override { return "variable"; }
};

// A constant typed by the author.
class LiteralNode : public ValueNode
{
    Q_OBJECT
public:
    QString name() const override { return "literal"; }
    QString caption() const override { return "Value"; }

protected:
    [[nodiscard]] QString placeholder() const override { return "value"; }
};

#endif //EDITOR_VALUE_NODE_H
