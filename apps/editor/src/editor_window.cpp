#include "editor_window.h"

#include <QAction>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QListWidget>
#include <QMenuBar>
#include <QAbstractItemView>
#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QStandardPaths>
#include <QStyle>
#include <QToolBar>
#include <QListView>
#include <QTabWidget>

#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "details_panel.h"
#include "graph_document.h"
#include "graph_model.h"
#include "graph_scene.h"
#include "graph_view.h"
#include "node_adaptor.h"
#include "node_geometry.h"
#include "value_tree.h"

#include "loom/graph/validate.h"
#include "loom/nodes/builtin.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    // Where a file dialog opens when the story it is about has no path yet.
    QString storyFolder(const QString& current)
    {
        return current.isEmpty() ? QString(LOOM_STORIES_DIR) : current;
    }

    // Deployed side by side; in a build tree each target has its own directory.
    QString findGame()
    {
#ifdef Q_OS_WIN
        const QString name = "LoomGame.exe";
#else
        const QString name = "LoomGame";
#endif

        const QString here = QCoreApplication::applicationDirPath();

        for (const QString& candidate : { here + "/" + name, here + "/../player/" + name })
        {
            if (QFileInfo::exists(candidate)) return QFileInfo(candidate).absoluteFilePath();
        }

        return QString();
    }

    // A green play triangle. The stock media icon is dark on a dark toolbar.
    QIcon playIcon()
    {
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(104, 198, 104));

        QPolygonF triangle;
        triangle << QPointF(6.0, 3.5) << QPointF(19.5, 12.0) << QPointF(6.0, 20.5);

        painter.drawPolygon(triangle);

        return QIcon(pixmap);
    }
}

EditorWindow::EditorWindow()
{
    loom::registerBuiltinNodes(catalog);

    setWindowTitle("Loom Editor");
    resize(1600, 900);

    buildCanvas();
    buildMenus();
    buildToolBar();
    buildDocks();

    newStory();
}

EditorWindow::~EditorWindow()
{
    // The view reads the scene and the model, and both are destroyed before it.
    delete takeCentralWidget();
}

void EditorWindow::buildCanvas()
{
    registry = makeRegistry(catalog, variableSpecs);
    model    = std::make_unique<GraphModel>(registry);

    scene = new GraphScene(*model, catalog, this);
    scene->setNodeGeometry(std::make_unique<NodeGeometry>(*model));

    view = new GraphView(scene, catalog, this);

    document = std::make_unique<GraphDocument>(*model, catalog);

    setCentralWidget(view);
}

void EditorWindow::buildMenus()
{
    QMenu* file = menuBar()->addMenu("&File");

    QAction* create = file->addAction("&New", this, &EditorWindow::newStory);
    create->setShortcut(QKeySequence::New);

    QAction* open = file->addAction("&Open Story...", this, &EditorWindow::chooseStory);
    open->setShortcut(QKeySequence::Open);

    file->addSeparator();

    saveAction = file->addAction("&Save", this, &EditorWindow::saveStory);
    saveAction->setShortcut(QKeySequence::Save);

    QAction* saveAs = file->addAction("Save &As...", this, &EditorWindow::saveStoryAs);
    saveAs->setShortcut(QKeySequence::SaveAs);

    file->addSeparator();

    QAction* quit = file->addAction("&Quit", this, &QWidget::close);
    quit->setShortcut(QKeySequence::Quit);

    QMenu* edit = menuBar()->addMenu("&Edit");

    QAction* undo = scene->undoStack().createUndoAction(this, "&Undo");
    undo->setShortcut(QKeySequence::Undo);

    QAction* redo = scene->undoStack().createRedoAction(this, "&Redo");
    redo->setShortcut(QKeySequence::Redo);

    edit->addAction(undo);
    edit->addAction(redo);

    QMenu* debug = menuBar()->addMenu("&Debug");

    playAction = debug->addAction("&Play", this, &EditorWindow::playStory);
    playAction->setShortcut(Qt::Key_F5);

    QAction* here = debug->addAction("Play From &Here");
    here->setShortcut(Qt::SHIFT | Qt::Key_F5);
    here->setEnabled(false);

    debug->addSeparator();

    debug->addAction("&Clear Console", this, &EditorWindow::clearConsole);
}

void EditorWindow::buildToolBar()
{
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    playAction->setIcon(playIcon());

    QToolBar* bar = addToolBar("Main");
    bar->setMovable(false);
    bar->setIconSize(QSize(24, 24));

    // Placeholder styling.
    bar->setStyleSheet(
        "QToolBar { padding: 5px 8px; spacing: 14px; }"
        "QToolButton { padding: 6px; border: 1px solid transparent; border-radius: 4px; }"
        "QToolButton:hover { background: #3d3d3d; border-color: #5a5a5a; }"
        "QToolButton:pressed { background: #2b2b2b; }");

    bar->addAction(saveAction);
    bar->addAction(playAction);
}

QWidget* EditorWindow::buildConsole()
{
    console = new QListWidget;
    console->setMinimumHeight(140);

    connect(console, &QListWidget::itemDoubleClicked, this, &EditorWindow::revealNode);

    return console;
}

QWidget* EditorWindow::buildScenes()
{
    scenes = new QListWidget;
    scenes->setFlow(QListView::LeftToRight);
    scenes->setWrapping(false);
    scenes->setFixedHeight(34);
    scenes->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scenes->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    scenes->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(scenes, &QListWidget::currentRowChanged, this, &EditorWindow::switchScene);
    connect(scenes, &QListWidget::itemChanged, this, &EditorWindow::renameScene);

    connect(scenes, &QListWidget::customContextMenuRequested, this, [this](const QPoint& at)
    {
        QListWidgetItem* item = scenes->itemAt(at);
        if (item == nullptr) return;

        QMenu menu;
        QAction* start = menu.addAction("Start Here");

        if (menu.exec(scenes->mapToGlobal(at)) != start) return;

        project.entry = project.graphs[scenes->row(item)].name;
        refreshScenes();
    });

    QPushButton* add = new QPushButton("+");
    QPushButton* remove = new QPushButton("-");

    add->setFixedWidth(28);
    remove->setFixedWidth(28);

    connect(add, &QPushButton::clicked, this, &EditorWindow::addScene);
    connect(remove, &QPushButton::clicked, this, &EditorWindow::removeScene);

    QWidget* strip = new QWidget;

    QHBoxLayout* row = new QHBoxLayout(strip);
    row->setContentsMargins(4, 4, 4, 4);
    row->setSpacing(4);
    row->addWidget(add);
    row->addWidget(remove);
    row->addWidget(scenes, 1);

    return strip;
}

QWidget* EditorWindow::buildPanel()
{
    panel = new QTabWidget;
    panel->setDocumentMode(true);

    details = new DetailsPanel;
    values  = new ValueTree;

    connect(values, &ValueTree::changed, this, &EditorWindow::syncVariableNames);
    connect(values, &ValueTree::renamed, this, &EditorWindow::renameVariable);
    connect(values, &ValueTree::removed, this, &EditorWindow::reportVariableUses);

    panel->addTab(values, "Variables");
    panel->addTab(details, "Details");

    return panel;
}

void EditorWindow::syncVariableNames()
{
    std::map<std::string, loom::VariableSpec> declared = values->variables();

    // A node offers a name and the type beside it, and takes the pin type from
    // that same type, so both decide whether the menus are stale. Typing a new
    // starting value changes neither and leaves them alone.
    std::vector<std::pair<std::string, std::string>> menu;

    for (const std::string& path : loom::variablePaths(declared))
    {
        menu.emplace_back(path, loom::declaredTypeAt(declared, path));
    }

    variableSpecs = std::move(declared);

    if (menu == offeredMenu) return;

    offeredMenu = std::move(menu);

    for (QtNodes::NodeId id : model->allNodeIds())
    {
        if (NodeAdaptor* node = model->delegateModel<NodeAdaptor>(id)) node->refreshEditors();
    }
}

void EditorWindow::forEachVariableUse(const std::string& named, const VariableUse& visit)
{
    // The canvas is ahead of the project, and nodes added since the last save
    // would otherwise be missed.
    project.graphs[editing] = document->graph();

    for (std::size_t at = 0; at < project.graphs.size(); ++at)
    {
        loom::Graph& graph = project.graphs[at];

        for (loom::NodeInstance& node : graph.nodes)
        {
            const loom::NodeType* type = catalog.find(node.type);
            if (type == nullptr) continue;

            for (const loom::PinSpec& pin : type->pins(node.extraPins))
            {
                if (pin.type != loom::PinType::VariableName) continue;

                const auto stored = node.pinValues.find(pin.name);
                if (stored == node.pinValues.end()) continue;

                // A field nested under the name is named by it too.
                const std::string chosen = loom::asString(stored->second);

                if (chosen != named && chosen.rfind(named + ".", 0) != 0) continue;

                visit(at, graph, node, pin);
            }
        }
    }
}

void EditorWindow::renameVariable(const QString& before, const QString& after)
{
    const std::string was = before.toStdString();
    const std::string now = after.toStdString();

    log("Renamed the variable '" + before + "' to '" + after + "'");

    forEachVariableUse(was,
                       [&](std::size_t at, loom::Graph& graph, loom::NodeInstance& node,
                           const loom::PinSpec& pin)
    {
        // Only the head of the path moved; whatever was nested under it stays.
        const std::string chosen = loom::asString(node.pinValues[pin.name]);
        const std::string moved  = now + chosen.substr(was.size());

        if (at == editing)
        {
            NodeAdaptor* live =
                model->delegateModel<NodeAdaptor>(static_cast<QtNodes::NodeId>(node.id));

            if (live != nullptr) live->setPinValue(pin.name, moved);
        }

        node.pinValues[pin.name] = moved;

        logAt("  Updated " + nodeLabel(graph.name, node.id) + " in scene '" +
              QString::fromStdString(graph.name) + "'",
              graph.name, node.id);
    });
}

void EditorWindow::reportVariableUses(const QString& name)
{
    int found = 0;

    // The nodes keep the name they were given; only the console says anything.
    forEachVariableUse(name.toStdString(),
                       [&](std::size_t, loom::Graph& graph, loom::NodeInstance& node,
                           const loom::PinSpec&)
    {
        if (found++ == 0)
        {
            log("The variable '" + name + "' is gone, but it is still named by:", true);
        }

        logAt("  " + nodeLabel(graph.name, node.id) + " in scene '" +
              QString::fromStdString(graph.name) + "'",
              graph.name, node.id, true);
    });
}

void EditorWindow::buildDocks()
{
    // Corners decide whether the left column or the bottom bar owns them.
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    QDockWidget* playtest = new QDockWidget("Playtest", this);
    playtest->setWidget(new QWidget);
    addDockWidget(Qt::LeftDockWidgetArea, playtest);

    QDockWidget* output = new QDockWidget("Console", this);
    output->setWidget(buildConsole());
    addDockWidget(Qt::LeftDockWidgetArea, output);

    splitDockWidget(playtest, output, Qt::Vertical);

    QDockWidget* strip = new QDockWidget("Scenes", this);
    strip->setWidget(buildScenes());
    addDockWidget(Qt::BottomDockWidgetArea, strip);

    QDockWidget* inspector = new QDockWidget("Inspector", this);
    inspector->setWidget(buildPanel());

    addDockWidget(Qt::RightDockWidgetArea, inspector);

    resizeDocks({ playtest, output }, { 420, 300 }, Qt::Vertical);
    resizeDocks({ playtest, output, inspector }, { 320, 320, 300 }, Qt::Horizontal);

    connect(scene, &QGraphicsScene::selectionChanged, this, &EditorWindow::syncDetails);

    syncDetails();
}

void EditorWindow::syncDetails()
{
    NodeAdaptor* only = nullptr;

    for (QGraphicsItem* item : scene->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        // The panel holds one node, so a wider selection shows none.
        if (only != nullptr)
        {
            only = nullptr;
            break;
        }

        only = model->delegateModel<NodeAdaptor>(object->nodeId());
    }

    details->setNode(only);

    if (only != nullptr) panel->setCurrentWidget(details);
}

void EditorWindow::refreshScenes()
{
    rebuilding = true;

    scenes->clear();

    for (const loom::Graph& graph : project.graphs)
    {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(graph.name), scenes);
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        if (graph.name == project.entry)
        {
            QFont bold = item->font();
            bold.setBold(true);

            item->setFont(bold);
        }
    }

    scenes->setCurrentRow(static_cast<int>(editing));

    rebuilding = false;
}

void EditorWindow::showScene(const loom::Graph& graph)
{
    // Painting is off while the canvas empties and refills.
    view->setUpdatesEnabled(false);

    document->open(graph);

    view->setUpdatesEnabled(true);

    scene->undoStack().clear();

    syncDetails();
}

void EditorWindow::switchScene(int index)
{
    if (rebuilding || index < 0) return;

    const std::size_t wanted = static_cast<std::size_t>(index);

    if (wanted == editing || wanted >= project.graphs.size()) return;

    project.graphs[editing] = document->graph();

    editing = wanted;

    showScene(project.graphs[editing]);
}

void EditorWindow::addScene()
{
    project.graphs[editing] = document->graph();

    loom::Graph fresh;
    fresh.name = uniqueSceneName("scene");
    fresh.meta = project.meta;

    project.graphs.push_back(fresh);

    editing = project.graphs.size() - 1;

    document->reset();
    document->setName(fresh.name);

    scene->undoStack().clear();

    syncDetails();
    refreshScenes();
}

void EditorWindow::removeScene()
{
    if (project.graphs.size() < 2)
    {
        log("A story needs at least one scene.", true);
        return;
    }

    const std::string gone = project.graphs[editing].name;

    project.graphs.erase(project.graphs.begin() + static_cast<std::ptrdiff_t>(editing));

    if (editing >= project.graphs.size()) editing = project.graphs.size() - 1;

    if (project.findGraph(project.entry) == nullptr) project.entry = project.graphs.front().name;

    showScene(project.graphs[editing]);

    refreshScenes();

    log("Removed the scene '" + QString::fromStdString(gone) + "'");

    reportSceneReferences(gone);
}

void EditorWindow::renameScene(QListWidgetItem* item)
{
    if (rebuilding || item == nullptr) return;

    const std::size_t index = static_cast<std::size_t>(scenes->row(item));

    if (index >= project.graphs.size()) return;

    const std::string was = project.graphs[index].name;
    const std::string typed = item->text().toStdString();

    if (typed == was) return;

    const std::string now = uniqueSceneName(typed);

    project.graphs[index].name = now;

    if (project.entry == was) project.entry = now;
    if (index == editing) document->setName(now);

    refreshScenes();

    project.graphs[editing] = document->graph();

    reportSceneReferences(was);
}

std::string EditorWindow::uniqueSceneName(const std::string& wanted) const
{
    const std::string base = wanted.empty() ? "scene" : wanted;

    std::string name = base;

    for (int suffix = 2; project.findGraph(name) != nullptr; ++suffix)
    {
        name = base + std::to_string(suffix);
    }

    return name;
}

void EditorWindow::reportSceneReferences(const std::string& name)
{
    for (const loom::Graph& graph : project.graphs)
    {
        for (const loom::NodeInstance& node : graph.nodes)
        {
            for (const auto& stored : node.pinValues)
            {
                if (!loom::isString(stored.second)) continue;
                if (loom::asString(stored.second) != name) continue;

                logAt("Warning in scene '" + QString::fromStdString(graph.name) + "' at " +
                      nodeLabel(graph.name, node.id) + ", pin " +
                      QString::fromStdString(stored.first) + ": still names the scene '" +
                      QString::fromStdString(name) + "'",
                      graph.name, node.id);
            }
        }
    }
}

QListWidgetItem* EditorWindow::log(const QString& text, bool fault)
{
    QListWidgetItem* line = new QListWidgetItem(text, console);
    if (fault) line->setForeground(Qt::red);

    console->scrollToBottom();

    return line;
}

void EditorWindow::logAt(const QString& text, const std::string& graph, loom::NodeId node, bool fault)
{
    QListWidgetItem* line = log(text, fault);

    line->setData(Qt::UserRole, QString::fromStdString(graph));
    line->setData(Qt::UserRole + 1, node);
}

QString EditorWindow::nodeLabel(const std::string& graph, loom::NodeId node) const
{
    const QString number = " #" + QString::number(node);

    // The canvas is ahead of the project until the next save.
    if (graph.empty() || graph == project.graphs[editing].name)
    {
        NodeAdaptor* live = model->delegateModel<NodeAdaptor>(static_cast<QtNodes::NodeId>(node));

        if (live != nullptr) return live->caption() + number;
    }

    const loom::Graph* found = project.findGraph(graph);

    if (found != nullptr)
    {
        for (const loom::NodeInstance& instance : found->nodes)
        {
            if (instance.id != node) continue;

            const loom::NodeType* type = catalog.find(instance.type);

            if (type != nullptr) return QString::fromStdString(type->displayName()) + number;
        }
    }

    return "node" + number;
}

void EditorWindow::revealNode(QListWidgetItem* line)
{
    const QVariant target = line->data(Qt::UserRole + 1);

    if (!target.isValid()) return;

    const std::string wanted = line->data(Qt::UserRole).toString().toStdString();

    if (!wanted.empty() && wanted != project.graphs[editing].name)
    {
        std::size_t index = project.graphs.size();

        for (std::size_t at = 0; at < project.graphs.size(); ++at)
        {
            if (project.graphs[at].name == wanted) index = at;
        }

        if (index == project.graphs.size())
        {
            log("There is no longer a scene called '" + QString::fromStdString(wanted) + "'.", true);
            return;
        }

        scenes->setCurrentRow(static_cast<int>(index));
    }

    QtNodes::NodeGraphicsObject* object =
        scene->nodeGraphicsObject(static_cast<QtNodes::NodeId>(target.toInt()));

    if (object == nullptr)
    {
        log("Node #" + target.toString() + " is no longer on the canvas.", true);
        return;
    }

    scene->clearSelection();
    object->setSelected(true);

    view->centerOn(object);
}

void EditorWindow::report(const loom::Diagnostics& diagnostics)
{
    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        const bool fault = entry.severity == loom::Severity::Error;

        QString line = fault ? "Error" : "Warning";

        if (entry.node != 0)    line += " at " + nodeLabel(entry.graph, entry.node);
        if (!entry.pin.empty()) line += ", pin " + QString::fromStdString(entry.pin);

        line += ": " + QString::fromStdString(entry.message);

        if (entry.node == 0) log(line, fault);
        else                 logAt(line, entry.graph, entry.node, fault);
    }
}

void EditorWindow::newStory()
{
    document->reset();

    project = loom::Project();
    project.entry = document->name();

    loom::Graph first;
    first.name = document->name();

    project.graphs.push_back(first);

    editing = 0;

    scene->undoStack().clear();
    console->clear();

    syncDetails();

    values->setVariables(project.variables);
    syncVariableNames();

    setStoryPath(QString());
    refreshScenes();
}

void EditorWindow::chooseStory()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Story", storyFolder(storyPath),
                                                      "Loom stories (*.loom);;All files (*)");

    if (!path.isEmpty()) openStory(path);
}

void EditorWindow::openStory(const QString& path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        log("Cannot read " + path, true);
        return;
    }

    loom::Value parsed;
    std::string error;

    if (!loom::parseJson(file.readAll().toStdString(), parsed, error))
    {
        log(QString::fromStdString(error), true);
        return;
    }

    loom::Project     opened;
    loom::Diagnostics diagnostics;
    bool              read = false;

    if (loom::objectGet(parsed, "graphs") != nullptr)
    {
        read = loom::readProject(parsed, catalog, opened, diagnostics);
    }
    else
    {
        // A single graph file is a project with one scene in it.
        loom::Graph graph;
        read = loom::readGraph(parsed, catalog, graph, diagnostics);

        opened.meta  = graph.meta;
        opened.entry = graph.name;
        opened.graphs.push_back(graph);
    }

    if (!read || diagnostics.hasErrors() || opened.graphs.empty())
    {
        report(diagnostics);
        log("This story was not opened.", true);
        return;
    }

    project = opened;
    editing = 0;

    for (std::size_t index = 0; index < project.graphs.size(); ++index)
    {
        if (project.graphs[index].name == project.entry) editing = index;
    }

    // Before the canvas, whose nodes read the names as they build their menus.
    values->setVariables(project.variables);
    syncVariableNames();

    showScene(project.graphs[editing]);

    setStoryPath(path);
    refreshScenes();

    report(diagnostics);
    log("Opened " + path);
}

bool EditorWindow::saveStory()
{
    return storyPath.isEmpty() ? saveStoryAs() : writeStory(storyPath);
}

bool EditorWindow::saveStoryAs()
{
    const QString path = QFileDialog::getSaveFileName(this, "Save Story", storyFolder(storyPath),
                                                      "Loom stories (*.loom)");

    return path.isEmpty() ? false : writeStory(path);
}

bool EditorWindow::writeProjectTo(const QString& path)
{
    project.graphs[editing] = document->graph();
    project.variables = values->variables();

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        log("Cannot write " + path, true);
        return false;
    }

    file.write(QByteArray::fromStdString(loom::writeJson(loom::writeProject(project))));
    file.close();

    return true;
}

bool EditorWindow::writeStory(const QString& path)
{
    if (!writeProjectTo(path)) return false;

    setStoryPath(path);

    // Unfinished work is saved all the same, and every fault in it is listed.
    loom::Diagnostics diagnostics;
    loom::validate(project, catalog, diagnostics);

    report(diagnostics);
    log("Saved " + path);

    return true;
}

void EditorWindow::setStoryPath(const QString& path)
{
    storyPath = path;

    const QString shown = path.isEmpty() ? QString("Untitled") : QFileInfo(path).fileName();

    setWindowTitle(shown + " - Loom Editor");
}

void EditorWindow::playStory()
{
    const QString path =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/loom_playtest.loom";

    if (!writeProjectTo(path)) return;

    const QString player = findGame();

    if (player.isEmpty())
    {
        log("Cannot find LoomGame next to the editor.", true);
        return;
    }

    if (!QProcess::startDetached(player, { path }))
    {
        log("The player would not start.", true);
        return;
    }

    log("Playing " + QString::fromStdString(project.entry));
}

void EditorWindow::clearConsole()
{
    console->clear();
}
