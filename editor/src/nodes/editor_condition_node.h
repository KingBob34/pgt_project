#ifndef EDITOR_CONDITION_NODE_H
#define EDITOR_CONDITION_NODE_H
#include "nodes/editor_node.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector>

// Branches on game state. The expression lives inside the node: one combiner
// (all / any / not) over a list of comparisons.
class ConditionNode : public EditorNode
{
    Q_OBJECT
public:
    ConditionNode()
    {
        setHeaderColor(QColor(55, 75, 105));
    }

    [[nodiscard]] QString name() const override
    {
        return "condition";
    }

    [[nodiscard]] QString caption() const override
    {
        return "Condition";
    }

    [[nodiscard]] unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::In ? 1 : 2;
    }

    [[nodiscard]] QString portCaption(QtNodes::PortType portType,
                                      QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) return "in";
        return portIndex == 0 ? "true" : "false";
    }

    // Built on first use, never in the constructor
    QWidget* embeddedWidget() override
    {
        if (editor == nullptr) editor = createEditor();
        return editor;
    }

private:
    QWidget* createEditor()
    {
        auto* widget = new QWidget();

        auto* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(3);

        // { "all": [...] } / { "any": [...] } / { "not": {...} }
        combiner = new QComboBox(widget);
        combiner->addItems({"all", "any", "not"});
        layout->addWidget(combiner);

        leafLayout = new QVBoxLayout();
        leafLayout->setContentsMargins(0, 0, 0, 0);
        leafLayout->setSpacing(3);
        layout->addLayout(leafLayout);

        auto* buttons = new QHBoxLayout();
        buttons->setContentsMargins(0, 0, 0, 0);

        auto* add = new QPushButton("+", widget);
        auto* remove = new QPushButton("-", widget);
        add->setFixedSize(20, 20);
        remove->setFixedSize(20, 20);
        connect(add, &QPushButton::clicked, this, &ConditionNode::addLeaf);
        connect(remove, &QPushButton::clicked, this, &ConditionNode::removeLeaf);

        buttons->addWidget(add);
        buttons->addWidget(remove);
        buttons->addStretch();
        layout->addLayout(buttons);

        addLeaf();
        return widget;
    }

    // One leaf of the tree: { "var": ..., "op": ..., "value": ... }
    void addLeaf()
    {
        auto* leaf = new QWidget();

        auto* layout = new QHBoxLayout(leaf);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(3);

        auto* variable = new QLineEdit(leaf);
        variable->setPlaceholderText("variable");
        variable->setFixedWidth(90);

        // Operators accepted by the loader
        auto* op = new QComboBox(leaf);
        op->addItems({"==", "!=", ">", ">=", "<", "<=", "contains"});

        auto* value = new QLineEdit(leaf);
        value->setPlaceholderText("value");
        value->setFixedWidth(60);

        layout->addWidget(variable);
        layout->addWidget(op);
        layout->addWidget(value);

        leafLayout->addWidget(leaf);
        leaves.append(leaf);

        Q_EMIT requestNodeUpdate();
    }

    void removeLeaf()
    {
        if (leaves.size() <= 1) return;

        delete leaves.takeLast();
        Q_EMIT requestNodeUpdate();
    }

    QWidget* editor = nullptr;
    QComboBox* combiner = nullptr;
    QVBoxLayout* leafLayout = nullptr;
    QVector<QWidget*> leaves;
};

#endif //EDITOR_CONDITION_NODE_H
