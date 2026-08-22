#include "graph_scene.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWidgetAction>

#include <QtNodes/UndoCommands>
#include <QGraphicsProxyWidget>

#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "node_adaptor.h"

#include "loom/qt/convert.h"

using loom::qt::toQt;

namespace
{
    // Below the nodes and below the wires, which sit at -1.
    constexpr double kFrameLayer = -2.0;

    // Carries the machine name while the tree shows the display name, and the
    // pin a waiting wire would be joined to.
    constexpr int kTypeNameRole = Qt::UserRole + 1;
    constexpr int kLandingRole = Qt::UserRole + 2;

    // Tall and narrow, so most of the catalog is on screen at once. A step is
    // all the nesting needs: there is only ever one level of it.
    constexpr int kMenuWidth = 152;
    constexpr int kMenuHeight = 560;
    constexpr int kMenuIndent = 10;
}

GraphScene::GraphScene(QtNodes::DataFlowGraphModel& model, const loom::NodeCatalog& nodeCatalog,
                       QObject* parent)
    : QtNodes::DataFlowGraphicsScene(model, parent)
    , catalog(nodeCatalog)
{
    connect(&model, &QtNodes::AbstractGraphModel::nodeCreated, this,
            [this](QtNodes::NodeId) { prepareNodes(); });

    connect(&model, &QtNodes::AbstractGraphModel::nodeUpdated, this, &GraphScene::refitWidget);

    // A reset builds the graphics objects itself and announces none of them.
    connect(&model, &QtNodes::AbstractGraphModel::modelReset, this, &GraphScene::prepareNodes);

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

void GraphScene::prepareNodes()
{
    for (QtNodes::NodeId node : graphModel().allNodeIds())
    {
        QtNodes::NodeGraphicsObject* drawn = nodeGraphicsObject(node);
        if (drawn == nullptr) continue;

        // QtNodes answers a position change by moving the wires that reach the
        // node, but never asks to be told about one, so only a change it makes
        // itself during a drag gets through. A node moved by the model -- on
        // load, on undo, on the snap at the end of a drag -- leaves its wires
        // where they were, and the card draws the stretch it is standing on
        // from that stale curve.
        drawn->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

        // A node keeps the picture it last painted and blits it when it moves,
        // which would freeze that same stretch of wire inside it. A card is
        // cheap enough to paint every frame.
        drawn->setCacheMode(QGraphicsItem::NoCache);

        // The drop shadow QtNodes hangs on a node keeps a picture of its own,
        // and that one is only thrown away when the node itself asks to be
        // redrawn. A wire moving is not the node changing, so the shadow would
        // keep handing back the stretch of wire the node held a moment ago.
        drawn->setGraphicsEffect(nullptr);

    }

    sinkFrames();
}

void GraphScene::refitWidget(QtNodes::NodeId node)
{
    QtNodes::NodeGraphicsObject* drawn = nodeGraphicsObject(node);
    if (drawn == nullptr) return;

    for (QGraphicsItem* child : drawn->childItems())
    {
        QGraphicsProxyWidget* held = qgraphicsitem_cast<QGraphicsProxyWidget*>(child);

        if (held == nullptr || held->widget() == nullptr) continue;

        const QSizeF want = held->widget()->size();

        if (held->size() == want) continue;

        // The proxy took the widget's limits when it was handed the widget and
        // never asks again, so a plain resize is clamped to the size the rows
        // used to add up to. The floor is dropped first, or raising the ceiling
        // through it is refused.
        held->setMinimumSize(0.0, 0.0);
        held->setMaximumSize(want);
        held->setMinimumSize(want);
        held->resize(want);
    }
}

void GraphScene::sinkFrames()
{
    for (QtNodes::NodeId node : graphModel().allNodeIds())
    {
        QtNodes::NodeGraphicsObject* drawn = nodeGraphicsObject(node);

        if (drawn == nullptr || drawn->zValue() == kFrameLayer) continue;

        // A frame is drawn round other nodes, so it belongs under them, and
        // under the wires that run between them.
        const NodeAdaptor* adaptor = adaptorFor(graphModel(), node);

        if (adaptor != nullptr && adaptor->isFrame()) drawn->setZValue(kFrameLayer);
    }
}

QMenu* GraphScene::createSceneMenu(QPointF scenePos)
{
    return buildNodeMenu(scenePos, kNoWire);
}

QMenu* GraphScene::createWireMenu(QPointF scenePos, const QtNodes::ConnectionId& draft)
{
    return buildNodeMenu(scenePos, draft);
}

QtNodes::PortType GraphScene::wantedSide(const QtNodes::ConnectionId& draft) const
{
    // The end that is missing is the end the new node has to supply.
    if (draft.outNodeId == QtNodes::InvalidNodeId && draft.inNodeId != QtNodes::InvalidNodeId)
    {
        return QtNodes::PortType::Out;
    }

    if (draft.inNodeId == QtNodes::InvalidNodeId && draft.outNodeId != QtNodes::InvalidNodeId)
    {
        return QtNodes::PortType::In;
    }

    return QtNodes::PortType::None;
}

std::string GraphScene::carriedType(const QtNodes::ConnectionId& draft)
{
    const QtNodes::PortType held = QtNodes::oppositePort(wantedSide(draft));

    if (held == QtNodes::PortType::None) return {};

    const NodeAdaptor* adaptor = adaptorFor(graphModel(), QtNodes::getNodeId(held, draft));

    if (adaptor == nullptr) return {};

    return adaptor->dataType(held, QtNodes::getPortIndex(held, draft)).id.toStdString();
}

void GraphScene::addNode(const QString& type, QPointF at, const QtNodes::ConnectionId& draft,
                         const std::string& landing)
{
    // The node and the wire that reached it are one thing the author did, so
    // one undo has to take both back.
    const bool wiring = !landing.empty();

    if (wiring) undoStack().beginMacro("Add Node");

    QtNodes::NodeId made = QtNodes::InvalidNodeId;

    // CreateCommand keeps the id it made to itself, and announces it.
    const QMetaObject::Connection watch =
        connect(&graphModel(), &QtNodes::AbstractGraphModel::nodeCreated, this,
                [&made](QtNodes::NodeId node) { made = node; });

    undoStack().push(new QtNodes::CreateCommand(this, type, at));

    disconnect(watch);

    if (!wiring) return;

    const NodeAdaptor* adaptor = adaptorFor(graphModel(), made);

    if (adaptor != nullptr)
    {
        const QtNodes::PortIndex index = adaptor->portIndex(wantedSide(draft), landing);

        if (index != QtNodes::InvalidPortIndex)
        {
            const QtNodes::ConnectionId wire =
                QtNodes::makeCompleteConnectionId(draft, made, index);

            // The catalog said this type has a pin for the wire; the model has
            // the last word, and a pin still waiting for its variable has no
            // type for it to check.
            if (graphModel().connectionPossible(wire))
            {
                undoStack().push(new QtNodes::ConnectCommand(this, wire));
            }
        }
    }

    undoStack().endMacro();
}

QMenu* GraphScene::buildNodeMenu(QPointF scenePos, const QtNodes::ConnectionId& draft)
{
    const QtNodes::PortType wanted = wantedSide(draft);
    const std::string carried = carriedType(draft);

    // A wire whose own pin has no settled type rules nothing out, so the whole
    // catalog is offered and the author draws the wire themselves.
    const bool narrowed = wanted != QtNodes::PortType::None && !carried.empty()
                       && carried != loom::PinType::Unset;

    const loom::PinDirection side = wanted == QtNodes::PortType::In ? loom::PinDirection::Input
                                                                   : loom::PinDirection::Output;

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

        std::string landing;

        if (narrowed)
        {
            landing = loom::landingPin(*type, carried, side);

            if (landing.empty()) continue;
        }

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
        item->setData(0, kLandingRole, toQt(landing));
    }

    tree->expandAll();

    connect(tree, &QTreeWidget::itemClicked, this,
            [this, menu, scenePos, draft](QTreeWidgetItem* item, int)
    {
        if (!(item->flags() & Qt::ItemIsSelectable)) return;

        addNode(item->data(0, kTypeNameRole).toString(), scenePos, draft,
                item->data(0, kLandingRole).toString().toStdString());

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
