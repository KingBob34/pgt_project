#include "graph_view.h"

#include <cmath>
#include <utility>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QGraphicsProxyWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSet>
#include <QWheelEvent>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/Definitions>
#include <QtNodes/UndoCommands>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "graph_scene.h"
#include "inline_edit.h"
#include "move_command.h"
#include "node_adaptor.h"
#include "node_metrics.h"
#include "rubber_band_style.h"

namespace
{
    // What one turn of the wheel reports, in eighths of a degree.
    constexpr int kWheelNotch = 120;

    // How QtNodes hands a copied piece of a graph to the clipboard.
    const char* const kGraphMime = "application/qt-nodes-graph";

    // The corner that resizes a node the author may resize.
    constexpr double kGripSize = 14.0;

    // Where a frame's title is written, and the box that edits it in place.
    // The gap matches the one the painter leaves round the title.
    constexpr double kFrameTitleGap = 10.0;
    constexpr double kTitleBand = 38.0;
    constexpr int kLeastTitleBox = 90;

    // The nearest crossing of the grid the canvas draws.
    QPointF onGrid(const QPointF& at)
    {
        return QPointF(std::round(at.x() / metrics::gridStep) * metrics::gridStep,
                       std::round(at.y() / metrics::gridStep) * metrics::gridStep);
    }

    // The paragraph editor a point on a node lands on, if it lands on one.
    QPlainTextEdit* paragraphAt(QGraphicsItem* item, QPointF scenePos)
    {
        QGraphicsProxyWidget* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

        if (proxy == nullptr || proxy->widget() == nullptr) return nullptr;

        QWidget* under = proxy->widget()->childAt(proxy->mapFromScene(scenePos).toPoint());

        // The pointer lands on a viewport, so the editor is looked for above it.
        for (; under != nullptr; under = under->parentWidget())
        {
            if (QPlainTextEdit* box = qobject_cast<QPlainTextEdit*>(under)) return box;
        }

        return nullptr;
    }
}

GraphView::GraphView(QtNodes::BasicGraphicsScene* scene, const loom::NodeCatalog& nodeCatalog,
                     QWidget* parent)
    : QtNodes::GraphicsView(scene, parent)
    , catalog(nodeCatalog)
{
    setDragMode(QGraphicsView::RubberBandDrag);

    // A node is caught when the band has all of it, not when it has a corner.
    setRubberBandSelectionMode(Qt::ContainsItemShape);

    // QtNodes hangs a set of actions on the view. Undo and Redo are on the
    // same keys the window's already answer to, and Qt settles a key claimed
    // twice by running neither -- which is why the menu worked and the
    // keyboard did not. Delete goes because a node is removed with Backspace,
    // and because the view's copy of it fires while a pin is being typed in.
    for (const QKeySequence& taken : { QKeySequence(QKeySequence::Undo),
                                       QKeySequence(QKeySequence::Redo),
                                       QKeySequence(QKeySequence::Delete),
                                       QKeySequence(QKeySequence::Cut),
                                       QKeySequence(QKeySequence::Copy),
                                       QKeySequence(QKeySequence::Paste),
                                       QKeySequence(Qt::CTRL | Qt::Key_D) })
    {
        for (QAction* claimed : actions())
        {
            if (claimed->shortcut() == taken) removeAction(claimed);
        }
    }

    // A wire that lands is a wire that found a port, and the menu is only for
    // the ones that did not.
    connect(&scene->graphModel(), &QtNodes::AbstractGraphModel::connectionCreated, this,
            [this](const QtNodes::ConnectionId&) { landed = true; });

    // The grid is drawn from the exposed rectangle, so it needs whole repaints.
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    RubberBandStyle* style = new RubberBandStyle;
    style->setParent(viewport());

    viewport()->setStyle(style);
}

void GraphView::mousePressEvent(QMouseEvent* event)
{
    // Alt on a pin cuts what reaches it, the way it does in Blueprint. Held
    // over anything else the press means what it always did.
    if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::AltModifier)
        && breakWiresAt(mapToScene(event->pos())))
    {
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        QtNodes::NodeId frame = QtNodes::InvalidNodeId;

        if (NodeAdaptor* grabbed = gripAt(mapToScene(event->pos()), frame))
        {
            sizing     = frame;
            sizingFrom = event->pos();
            sizingWas  = grabbed->boxSize();

            viewport()->setCursor(Qt::SizeFDiagCursor);

            event->accept();
            return;
        }
    }

    if (event->button() == Qt::RightButton)
    {
        panning   = true;
        panned    = false;
        panOrigin = event->pos();
        panLast   = event->pos();

        viewport()->setCursor(Qt::ClosedHandCursor);

        event->accept();
        return;
    }

    QtNodes::GraphicsView::mousePressEvent(event);

    // After the base class, which is what decides the selection this adds to.
    if (event->button() != Qt::LeftButton) return;

    // A press on a port hands the mouse to the wire being drawn from it.
    drafting = dynamic_cast<QtNodes::ConnectionGraphicsObject*>(scene()->mouseGrabberItem())
             != nullptr;
    landed = false;

    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return;

    for (QGraphicsItem* item : graph->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        const NodeAdaptor* adaptor = adaptorFor(graph->graphModel(), object->nodeId());

        if (adaptor != nullptr && adaptor->isFrame()) gatherFramed(object->nodeId());
    }
}

bool GraphView::breakWiresAt(const QPointF& scenePos)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return false;

    QtNodes::AbstractGraphModel& model = graph->graphModel();

    for (QGraphicsItem* item : graph->items(scenePos))
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        const QPointF onNode = object->sceneTransform().inverted().map(scenePos);

        for (QtNodes::PortType side : { QtNodes::PortType::In, QtNodes::PortType::Out })
        {
            const QtNodes::PortIndex port =
                graph->nodeGeometry().checkPortHit(object->nodeId(), side, onNode);

            if (port == QtNodes::InvalidPortIndex) continue;

            const std::unordered_set<QtNodes::ConnectionId> wires =
                model.connections(object->nodeId(), side, port);

            // A pin with nothing on it still swallows the press: the author
            // asked to clear it, and clearing it is what happened.
            if (wires.empty()) return true;

            // One step on the undo stack, because one gesture cut them all.
            graph->undoStack().beginMacro("Break Links");

            for (const QtNodes::ConnectionId& wire : wires)
            {
                graph->undoStack().push(new QtNodes::DisconnectCommand(graph, wire));
            }

            graph->undoStack().endMacro();

            return true;
        }
    }

    return false;
}

NodeAdaptor* GraphView::gripAt(const QPointF& scenePos, QtNodes::NodeId& frame)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return nullptr;

    QtNodes::AbstractGraphModel& model = graph->graphModel();

    for (QtNodes::NodeId node : model.allNodeIds())
    {
        NodeAdaptor* adaptor = adaptorFor(model, node);
        if (adaptor == nullptr || !adaptor->isResizable()) continue;

        QtNodes::NodeGraphicsObject* object = graph->nodeGraphicsObject(node);
        if (object == nullptr) continue;

        // A frame is exactly its box; every other node is its box plus a title
        // and the rows around it, so the corner is where it was drawn.
        const QSize held = adaptor->isFrame() ? adaptor->boxSize()
                                              : graph->nodeGeometry().size(node);
        const QPointF corner = object->pos() + QPointF(held.width(), held.height());

        if (QRectF(corner - QPointF(kGripSize, kGripSize), corner).contains(scenePos))
        {
            frame = node;
            return adaptor;
        }
    }

    return nullptr;
}

void GraphView::gatherFramed(QtNodes::NodeId frame)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();

    QtNodes::AbstractGraphModel& model = graph->graphModel();
    QtNodes::NodeGraphicsObject* pane = graph->nodeGraphicsObject(frame);

    if (pane == nullptr) return;

    const NodeAdaptor* adaptor = adaptorFor(model, frame);
    const QSize held = adaptor->boxSize();

    const QRectF inside(pane->pos(), QSizeF(held.width(), held.height()));

    for (QtNodes::NodeId node : model.allNodeIds())
    {
        if (node == frame) continue;

        QtNodes::NodeGraphicsObject* object = graph->nodeGraphicsObject(node);
        if (object == nullptr || object->isSelected()) continue;

        const QSize size = graph->nodeGeometry().size(node);
        const QRectF held(object->pos(), QSizeF(size.width(), size.height()));

        // Completely surrounded, the way the rubber band counts a node as caught.
        if (inside.contains(held)) object->setSelected(true);
    }
}

void GraphView::mouseMoveEvent(QMouseEvent* event)
{
    if (sizing != QtNodes::InvalidNodeId)
    {
        const QPointF grew = mapToScene(event->pos()) - mapToScene(sizingFrom);

        NodeAdaptor* frame = adaptorFor(nodeScene()->graphModel(), sizing);

        if (frame != nullptr)
        {
            // The node itself refuses anything under the size it declares.
            frame->setBoxSize(QSize(int(sizingWas.width() + grew.x()),
                                    int(sizingWas.height() + grew.y())));
        }

        event->accept();
        return;
    }

    if (panning)
    {
        const QPoint step = event->pos() - panLast;
        panLast = event->pos();

        // Scrolling rather than setSceneRect, which makes the view re-centre itself.
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - step.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - step.y());

        if ((event->pos() - panOrigin).manhattanLength() > QApplication::startDragDistance())
        {
            panned = true;
        }

        event->accept();
        return;
    }

    // Hovering a node raises it, and a frame that has been hovered has to go
    // back down before it is drawn over what it is meant to sit behind.
    if (GraphScene* ours = qobject_cast<GraphScene*>(nodeScene())) ours->sinkFrames();

    // Skips QtNodes, whose handler pans on a left drag.
    QGraphicsView::mouseMoveEvent(event);
}

void GraphView::endPan()
{
    panning = false;

    viewport()->unsetCursor();
}

void GraphView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        const bool wasPanning = panning;

        endPan();

        if (wasPanning)
        {
            event->accept();
            return;
        }
    }

    if (sizing != QtNodes::InvalidNodeId)
    {
        sizing = QtNodes::InvalidNodeId;

        viewport()->unsetCursor();

        event->accept();
        return;
    }

    const bool wasDrafting = drafting;

    drafting = false;

    // Read while the wire is still being held: the base class lets go of it
    // and takes it off the canvas.
    QtNodes::ConnectionId draft = kNoWire;

    if (const QtNodes::ConnectionGraphicsObject* wire =
            dynamic_cast<QtNodes::ConnectionGraphicsObject*>(scene()->mouseGrabberItem()))
    {
        draft = wire->connectionId();
    }

    QtNodes::GraphicsView::mouseReleaseEvent(event);

    if (event->button() != Qt::LeftButton) return;

    // Dropped on empty canvas. The author was reaching for a node that is not
    // there yet, so offer to make one.
    if (wasDrafting && !landed) openNodeMenu(event->pos(), draft);

    commitNodePositions();

    if (GraphScene* ours = qobject_cast<GraphScene*>(nodeScene())) ours->sinkFrames();
}

void GraphView::commitNodePositions()
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return;

    QtNodes::AbstractGraphModel& model = graph->graphModel();

    std::vector<QtNodes::NodeId> moved;
    QPointF by;

    for (QGraphicsItem* item : graph->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        // The drag never reached the model, so what is stored is still where
        // the node started and the difference is the whole of the drag.
        const QPointF stored =
            model.nodeData(object->nodeId(), QtNodes::NodeRole::Position).value<QPointF>();

        if (object->pos() == stored) continue;

        // The first one that moved lands on the grid and sets the distance the
        // rest travel, so a group keeps the shape the author gave it.
        if (moved.empty()) by = onGrid(object->pos()) - stored;

        moved.push_back(object->nodeId());
    }

    if (moved.empty()) return;

    graph->undoStack().push(new MoveNodesCommand(model, std::move(moved), by));
}

void GraphView::keyPressEvent(QKeyEvent* event)
{
    // Backspace removes what is selected, unless something on the canvas is
    // being typed into, where it has to go on removing characters.
    if (event->key() == Qt::Key_Backspace && !typing())
    {
        onDeleteSelectedObjects();

        event->accept();
        return;
    }

    QtNodes::GraphicsView::keyPressEvent(event);
}

bool GraphView::typing() const
{
    QGraphicsScene* graph = scene();

    return graph != nullptr
        && qgraphicsitem_cast<QGraphicsProxyWidget*>(graph->focusItem()) != nullptr;
}

void GraphView::keyReleaseEvent(QKeyEvent* event)
{
    QtNodes::GraphicsView::keyReleaseEvent(event);

    // The base class answers a Shift release by going back to panning.
    setDragMode(QGraphicsView::RubberBandDrag);
}

void GraphView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QtNodes::NodeId frame = QtNodes::InvalidNodeId;

    if (event->button() == Qt::LeftButton && frameTitleAt(mapToScene(event->pos()), frame))
    {
        renameFrame(frame);

        event->accept();
        return;
    }

    QtNodes::GraphicsView::mouseDoubleClickEvent(event);
}

bool GraphView::frameTitleAt(const QPointF& scenePos, QtNodes::NodeId& frame)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return false;

    for (QGraphicsItem* item : graph->items(scenePos))
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        // The topmost node decides. A frame sits under the nodes it was drawn
        // round, so one of those being here means the click was not for it.
        const NodeAdaptor* adaptor = adaptorFor(graph->graphModel(), object->nodeId());

        if (adaptor == nullptr || !adaptor->isFrame()) return false;

        // Only the strip the title is written on, not the pane below it.
        if (scenePos.y() - object->pos().y() > kTitleBand) return false;

        frame = object->nodeId();
        return true;
    }

    return false;
}

void GraphView::openNodeMenu(const QPoint& at, const QtNodes::ConnectionId& draft)
{
    GraphScene* ours = qobject_cast<GraphScene*>(nodeScene());

    QMenu* menu = ours != nullptr ? ours->createWireMenu(mapToScene(at), draft)
                                  : nodeScene()->createSceneMenu(mapToScene(at));

    if (menu != nullptr) menu->popup(mapToGlobal(at));
}

QByteArray GraphView::withoutEntryPoints(const QByteArray& document) const
{
    const QJsonObject whole = QJsonDocument::fromJson(document).object();

    QJsonArray keptNodes;
    QSet<qint64> left;

    for (const QJsonValue& entry : whole["nodes"].toArray())
    {
        const QJsonObject node = entry.toObject();
        const std::string named =
            node["internal-data"].toObject()["model-name"].toString().toStdString();

        const loom::NodeType* type = catalog.find(named);

        if (type != nullptr && type->isEntryPoint())
        {
            left.insert(node["id"].toInteger());
            continue;
        }

        keptNodes.append(entry);
    }

    if (left.isEmpty()) return document;

    QJsonArray keptWires;

    for (const QJsonValue& entry : whole["connections"].toArray())
    {
        const QJsonObject wire = entry.toObject();

        if (left.contains(wire["outNodeId"].toInteger())) continue;
        if (left.contains(wire["inNodeId"].toInteger())) continue;

        keptWires.append(entry);
    }

    QJsonObject trimmed = whole;
    trimmed["nodes"] = keptNodes;
    trimmed["connections"] = keptWires;

    return QJsonDocument(trimmed).toJson();
}

bool GraphView::isEntryPoint(QtNodes::NodeId node)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return false;

    const NodeAdaptor* adaptor = adaptorFor(graph->graphModel(), node);

    return adaptor != nullptr && adaptor->nodeType().isEntryPoint();
}

std::vector<QtNodes::NodeGraphicsObject*> GraphView::dropEntryPoints()
{
    std::vector<QtNodes::NodeGraphicsObject*> dropped;

    QtNodes::BasicGraphicsScene* graph = nodeScene();
    if (graph == nullptr) return dropped;

    for (QGraphicsItem* item : graph->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object != nullptr && isEntryPoint(object->nodeId())) dropped.push_back(object);
    }

    for (QtNodes::NodeGraphicsObject* object : dropped) object->setSelected(false);

    return dropped;
}

void GraphView::onCopySelectedObjects()
{
    const std::vector<QtNodes::NodeGraphicsObject*> dropped = dropEntryPoints();

    QtNodes::GraphicsView::onCopySelectedObjects();

    // Copying is not meant to change what the author had picked out.
    for (QtNodes::NodeGraphicsObject* object : dropped) object->setSelected(true);
}

void GraphView::onDuplicateSelectedObjects()
{
    const std::vector<QtNodes::NodeGraphicsObject*> dropped = dropEntryPoints();

    QtNodes::BasicGraphicsScene* graph = nodeScene();

    // Duplicating copies and then pastes. With nothing left to copy the paste
    // would go ahead all the same and put back whatever the clipboard was
    // holding, so the entry point on its own does nothing at all.
    if (graph == nullptr || graph->selectedItems().isEmpty())
    {
        for (QtNodes::NodeGraphicsObject* object : dropped) object->setSelected(true);
        return;
    }

    // The copies it makes become the selection, so nothing is put back.
    QtNodes::GraphicsView::onDuplicateSelectedObjects();
}

void GraphView::onPasteObjects()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* held = clipboard->mimeData();

    if (held == nullptr || !held->hasFormat(kGraphMime))
    {
        QtNodes::GraphicsView::onPasteObjects();
        return;
    }

    // What was copied by an older build, or by another window, may still carry
    // an entry point. It is taken out of the clipboard rather than deleted
    // afterwards, so the paste stays one step on the undo stack.
    const QByteArray original = held->data(kGraphMime);
    const QByteArray filtered = withoutEntryPoints(original);

    if (filtered == original)
    {
        QtNodes::GraphicsView::onPasteObjects();
        return;
    }

    QMimeData* trimmed = new QMimeData;
    trimmed->setData(kGraphMime, filtered);
    trimmed->setText(filtered);

    clipboard->setMimeData(trimmed);

    QtNodes::GraphicsView::onPasteObjects();

    QMimeData* restored = new QMimeData;
    restored->setData(kGraphMime, original);
    restored->setText(original);

    clipboard->setMimeData(restored);
}

void GraphView::onCutSelectedObjects()
{
    onCopySelectedObjects();
    onDeleteSelectedObjects();
}

void GraphView::renameFrame(QtNodes::NodeId frame)
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();

    NodeAdaptor* adaptor = adaptorFor(graph->graphModel(), frame);
    QtNodes::NodeGraphicsObject* object = graph->nodeGraphicsObject(frame);

    if (adaptor == nullptr || object == nullptr) return;

    // Edited where it is read. The box goes into the scene and hangs off the
    // frame itself, so it travels with it: one parented to the viewport stays
    // where it was drawn the moment the canvas is scrolled.
    QGraphicsProxyWidget* held = nullptr;

    InlineEdit* box = new InlineEdit(adaptor->caption(), [this, frame](const QString& typed)
    {
        if (NodeAdaptor* named = adaptorFor(nodeScene()->graphModel(), frame))
        {
            named->setPinValue("text", loom::Value(typed.toStdString()));
        }

        if (GraphScene* ours = qobject_cast<GraphScene*>(nodeScene()))
        {
            ours->setRenaming(QtNodes::InvalidNodeId);
        }
    });

    box->setFixedWidth(std::max(kLeastTitleBox,
                                adaptor->boxSize().width() - 2 * int(kFrameTitleGap)));

    // In the scene and hung off the frame, so it travels with it: a box
    // parented to the viewport stays put the moment the canvas is scrolled.
    held = graph->addWidget(box);
    held->setParentItem(object);
    held->setPos(kFrameTitleGap, kFrameTitleGap);
    held->setZValue(1.0);

    box->setFocus();
    box->selectAll();

    if (GraphScene* ours = qobject_cast<GraphScene*>(graph)) ours->setRenaming(frame);

    // The proxy owns the box, so taking the proxy away takes both.
    connect(box, &InlineEdit::finished, this, [held] { held->deleteLater(); });
}

void GraphView::contextMenuEvent(QContextMenuEvent* event)
{
    // The menu takes the mouse, so the release that would end the pan never
    // arrives here.
    const bool dragged = panned;

    endPan();

    // A dragged right button was a pan, not a request for the menu.
    if (dragged)
    {
        event->accept();
        return;
    }

    // Whatever is drawn topmost under the pointer answers for itself. A right
    // click on a node reaches QtNodes' own menu otherwise, which offers
    // grouping this editor does not use and a clipboard that drops pin values.
    for (QGraphicsItem* item : items(event->pos()))
    {
        if (qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item) != nullptr)
        {
            QMenu menu;
            QAction* twin = menu.addAction("Duplicate");
            QAction* remove = menu.addAction("Delete");

            const QAction* chosen = menu.exec(event->globalPos());

            if (chosen == twin) onDuplicateSelectedObjects();
            if (chosen == remove) onDeleteSelectedObjects();

            event->accept();
            return;
        }

        const QtNodes::ConnectionGraphicsObject* wire =
            qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item);

        if (wire != nullptr)
        {
            // Cut where it was clicked. A wire has one thing that can be done
            // to it, so a menu offering that one thing is a step in the way.
            nodeScene()->undoStack().push(
                new QtNodes::DisconnectCommand(nodeScene(), wire->connectionId()));

            event->accept();
            return;
        }
    }

    QtNodes::GraphicsView::contextMenuEvent(event);
}

void GraphView::wheelEvent(QWheelEvent* event)
{
    const QPoint at = event->position().toPoint();

    // A text box under the pointer takes the whole wheel, including the turns
    // that scroll nothing: reaching the last line must not start a zoom.
    if (QPlainTextEdit* box = paragraphAt(itemAt(at), mapToScene(at)))
    {
        QScrollBar* bar = box->verticalScrollBar();
        const int lines = event->angleDelta().y() * QApplication::wheelScrollLines() / kWheelNotch;

        bar->setValue(bar->value() - lines * bar->singleStep());

        event->accept();
        return;
    }

    QtNodes::GraphicsView::wheelEvent(event);
}

void GraphView::resizeEvent(QResizeEvent* event)
{
    QtNodes::GraphicsView::resizeEvent(event);

    const QPoint was = corner;
    corner = mapTo(window(), QPoint(0, 0));

    // The first layout is where the canvas is put, not somewhere it moved to.
    if (!event->oldSize().isValid()) return;

    // A panel on the left or the top does not take width off the canvas so
    // much as slide its corner inwards, and the view goes on showing the same
    // scene point at that corner. Scrolling by as far as the corner came in
    // leaves the graph where the author last saw it, which is what a panel on
    // the right does by itself.
    const QPoint moved = corner - was;

    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + moved.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() + moved.y());
}

void GraphView::onDeleteSelectedObjects()
{
    QtNodes::BasicGraphicsScene* graph = nodeScene();

    if (graph != nullptr)
    {
        QtNodes::AbstractGraphModel& model = graph->graphModel();

        const auto entryPoint = [&model](QtNodes::NodeId node)
        {
            const NodeAdaptor* adaptor = adaptorFor(model, node);

            return adaptor != nullptr && adaptor->nodeType().isEntryPoint();
        };

        std::vector<QtNodes::NodeGraphicsObject*> ways;

        for (QGraphicsItem* item : graph->selectedItems())
        {
            QtNodes::NodeGraphicsObject* object =
                qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

            if (object != nullptr && entryPoint(object->nodeId())) ways.push_back(object);
        }

        std::size_t inScene = 0;

        for (QtNodes::NodeId node : model.allNodeIds())
        {
            if (entryPoint(node)) ++inScene;
        }

        // A scene has to keep one way in, but a second one is a mistake -- a
        // pasted copy, usually -- and the author has to be able to get rid of
        // it. So only the last one left is protected.
        if (!ways.empty() && ways.size() >= inScene) ways.front()->setSelected(false);
    }

    QtNodes::GraphicsView::onDeleteSelectedObjects();
}
