#ifndef LOOM_EDITOR_VALUE_TREE_H
#define LOOM_EDITOR_VALUE_TREE_H
#include <map>
#include <string>

#include <QWidget>

#include "loom/graph/graph.h"

class QTreeWidget;
class QTreeWidgetItem;

// The globals an author declares: a name, a type and a starting value each.
class ValueTree : public QWidget
{
    Q_OBJECT

public:
    explicit ValueTree(QWidget* parent = nullptr);

    void setVariables(const std::map<std::string, loom::VariableSpec>& variables);
    std::map<std::string, loom::VariableSpec> variables() const;

    Q_SIGNALS:
        void changed();

        // A declared variable was given a new name; nodes still name the old one.
        void renamed(const QString& before, const QString& after);

        // A declared variable is gone; nodes may still name it.
        void removed(const QString& name);

private:
    void addVariable();
    void removeVariable();

    // A row under parent, or a top-level one when parent is null.
    QTreeWidgetItem* addRow(QTreeWidgetItem* parent, const std::string& name,
                            const std::string& type, const loom::Value& value);

    void fillRow(QTreeWidgetItem* row, const std::string& name,
                 const std::string& type, const loom::Value& value);

    // Builds the value column: an editor for a leaf, child rows for a container.
    void showValue(QTreeWidgetItem* row, const std::string& type);

    // Answers a type the author picked: the old value goes, a blank one arrives.
    void retype(QTreeWidgetItem* row, const std::string& type);

    // A container is worth whatever its rows are worth; a leaf holds its own.
    loom::Value valueOf(QTreeWidgetItem* row) const;
    std::string typeOf(QTreeWidgetItem* row) const;

    void refreshCount(QTreeWidgetItem* row);
    void renumber(QTreeWidgetItem* list);

    int              siblingCount(QTreeWidgetItem* parent) const;
    QTreeWidgetItem* sibling(QTreeWidgetItem* parent, int at) const;

    std::string uniqueName(QTreeWidgetItem* parent, const std::string& wanted) const;

    // The row's name with its ancestors in front, joined by dots.
    QString pathOf(QTreeWidgetItem* row) const;

    QTreeWidget* tree = nullptr;

    // True while rows are being built, so their own signals can be ignored.
    bool filling = false;
};

#endif //LOOM_EDITOR_VALUE_TREE_H
