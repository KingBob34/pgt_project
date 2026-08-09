#ifndef EDITOR_BOOLEAN_NODE_H
#define EDITOR_BOOLEAN_NODE_H
#include "nodes/editor_node.h"
#include <QHBoxLayout>
#include <QPushButton>

// AND / OR: a variable number of bool inputs, one bool output.
class BooleanNode : public EditorNode
{
    Q_OBJECT
public:
    BooleanNode()
    {
        setHeaderColor(QColor(75, 60, 100));
    }

    int minimumWidth() const override
    {
        return kOperatorNodeWidth;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? inputCount : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return boolPin();
    }

    bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return false;
    }

    // Built on first use, never in the constructor
    QWidget* embeddedWidget() override
    {
        if (buttons == nullptr) buttons = createButtons();
        return buttons;
    }

protected:
    void addInput()
    {
        const QtNodes::PortIndex added = inputCount;
        Q_EMIT portsAboutToBeInserted(QtNodes::PortType::In, added, added);
        inputCount++;
        Q_EMIT portsInserted();

        // Port signals repair the connections; this one resizes the node
        Q_EMIT requestNodeUpdate();
    }

    void removeInput()
    {
        if (inputCount <= 2) return;

        const QtNodes::PortIndex removed = inputCount - 1;
        Q_EMIT portsAboutToBeDeleted(QtNodes::PortType::In, removed, removed);
        inputCount--;
        Q_EMIT portsDeleted();

        Q_EMIT requestNodeUpdate();
    }

private:
    QWidget* createButtons()
    {
        auto* widget = new QWidget();

        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        auto* add = new QPushButton("+", widget);
        auto* remove = new QPushButton("-", widget);
        add->setFixedSize(20, 20);
        remove->setFixedSize(20, 20);

        connect(add, &QPushButton::clicked, this, &BooleanNode::addInput);
        connect(remove, &QPushButton::clicked, this, &BooleanNode::removeInput);

        layout->addWidget(add);
        layout->addWidget(remove);
        return widget;
    }

    unsigned int inputCount = 2;
    QWidget* buttons = nullptr;
};

class AndNode : public BooleanNode
{
    Q_OBJECT
public:
    QString name() const override { return "and"; }
    QString caption() const override { return "AND"; }
};

class OrNode : public BooleanNode
{
    Q_OBJECT
public:
    QString name() const override { return "or"; }
    QString caption() const override { return "OR"; }
};

// NOT: one bool input, one bool output.
class NotNode : public EditorNode
{
    Q_OBJECT
public:
    NotNode()
    {
        setHeaderColor(QColor(75, 60, 100));
    }

    QString name() const override { return "not"; }
    QString caption() const override { return "NOT"; }

    int minimumWidth() const override
    {
        return kOperatorNodeWidth;
    }

    unsigned int nPorts(QtNodes::PortType) const override
    {
        return 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return boolPin();
    }

    bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return false;
    }
};

#endif //EDITOR_BOOLEAN_NODE_H
