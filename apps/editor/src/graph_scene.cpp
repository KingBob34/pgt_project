#include "graph_scene.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWidgetAction>

#include <QtNodes/UndoCommands>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>

namespace
{
    // Carries the machine name while the tree shows the display name.
    constexpr int kTypeNameRole = Qt::UserRole + 1;

    // Tall and narrow, so most of the catalog is on screen at once. A step is
    // all the nesting needs: there is only ever one level of it.
    constexpr int kMenuWidth = 176;
    constexpr int kMenuHeight = 460;
    constexpr int kMenuIndent = 10;

    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }
}

GraphScene::GraphScene(QtNodes::DataFlowGraphModel& model, const loom::NodeCatalog& nodeCatalog,
                       QObject* parent)
    : QtNodes::DataFlowGraphicsScene(model, parent)
    , catalog(nodeCatalog)
{
    // A finished wire is drawn between two ports and has nowhere of its own to
    // be, but QtNodes leaves it movable and a drag carries it off both of them.
    connect(&model, &QtNodes::AbstractGraphModel::connectionCreated, this,
            [this](const QtNodes::ConnectionId& wire)
            {
                if (QtNodes::ConnectionGraphicsObject* drawn = connectionGraphicsObject(wire))
                {
                    drawn->setFlag(QGraphicsItem::ItemIsMovable, false);
                }
            });
}

QMenu* GraphScene::createSceneMenu(QPointF scenePos)
{
    QMenu* menu = new QMenu;
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QLineEdit* filter = new QLineEdit(menu);
    filter->setPlaceholderText("Filter");
    filter->setClearButtonEnabled(true);

    QWidgetAction* filterAction = new QWidgetAction(menu);
    filterAction->setDefaultWidget(filter);
    menu->addAction(filterAction);

    QTreeWidget* tree = new QTreeWidget(menu);
    tree->header()->close();
    tree->setFixedSize(kMenuWidth, kMenuHeight);
    tree->setIndentation(kMenuIndent);

    QWidgetAction* treeAction = new QWidgetAction(menu);
    treeAction->setDefaultWidget(tree);
    menu->addAction(treeAction);

    QMap<QString, QTreeWidgetItem*> groups;

    for (const loom::NodeType* type : catalog.all())
    {
        // A graph is created with its entry point and may only have one.
        if (type->isEntryPoint()) continue;

        const QString category = toQt(type->category());

        if (!groups.contains(category))
        {
            QTreeWidgetItem* group = new QTreeWidgetItem(tree);
            group->setText(0, category);
            group->setFlags(group->flags() & ~Qt::ItemIsSelectable);
            groups.insert(category, group);
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(groups.value(category));
        item->setText(0, toQt(type->displayName()));
        item->setData(0, kTypeNameRole, toQt(type->name()));
    }

    tree->expandAll();

    connect(tree, &QTreeWidget::itemClicked, this, [this, menu, scenePos](QTreeWidgetItem* item, int)
    {
        if (!(item->flags() & Qt::ItemIsSelectable)) return;

        undoStack().push(new QtNodes::CreateCommand(this, item->data(0, kTypeNameRole).toString(),
                                                    scenePos));
        menu->close();
    });

    connect(filter, &QLineEdit::textChanged, tree, [tree](const QString& text)
    {
        QTreeWidgetItemIterator groups(tree, QTreeWidgetItemIterator::HasChildren);
        while (*groups) (*groups++)->setHidden(true);

        QTreeWidgetItemIterator leaves(tree, QTreeWidgetItemIterator::NoChildren);
        while (*leaves)
        {
            const bool match = (*leaves)->text(0).contains(text, Qt::CaseInsensitive);
            (*leaves)->setHidden(!match);

            if (match && (*leaves)->parent() != nullptr) (*leaves)->parent()->setHidden(false);

            ++leaves;
        }
    });

    filter->setFocus();

    return menu;
}
