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

#include "graph_document.h"
#include "graph_model.h"
#include "graph_scene.h"
#include "graph_view.h"
#include "node_adaptor.h"
#include "node_geometry.h"

#include "loom/graph/validate.h"
#include "loom/nodes/builtin.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    // Deployed side by side; in a build tree each target has its own directory.
    QString findPlayer()
    {
#ifdef Q_OS_WIN
        const QString name = "loom_player.exe";
#else
        const QString name = "loom_player";
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
    resize(1400, 900);

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
    registry = makeRegistry(catalog);
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

    panel->addTab(new QWidget, "Variables");

    return panel;
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
    resizeDocks({ inspector }, { 320 }, Qt::Horizontal);
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

                log("Warning in scene '" + QString::fromStdString(graph.name) + "' at node " +
                    QString::number(node.id) + ", pin " + QString::fromStdString(stored.first) +
                    ": still names the scene '" + QString::fromStdString(name) + "'");
            }
        }
    }
}

void EditorWindow::log(const QString& text, bool fault)
{
    QListWidgetItem* line = new QListWidgetItem(text, console);
    if (fault) line->setForeground(Qt::red);

    console->scrollToBottom();
}

void EditorWindow::report(const loom::Diagnostics& diagnostics)
{
    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        const bool fault = entry.severity == loom::Severity::Error;

        QString line = fault ? "Error" : "Warning";

        if (entry.node != 0)    line += " at node " + QString::number(entry.node);
        if (!entry.pin.empty()) line += ", pin " + QString::fromStdString(entry.pin);

        log(line + ": " + QString::fromStdString(entry.message), fault);
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

    setStoryPath(QString());
    refreshScenes();
}

void EditorWindow::chooseStory()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Story", storyPath,
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
    const QString path = QFileDialog::getSaveFileName(this, "Save Story", storyPath,
                                                      "Loom stories (*.loom)");

    return path.isEmpty() ? false : writeStory(path);
}

bool EditorWindow::writeProjectTo(const QString& path)
{
    project.graphs[editing] = document->graph();

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

    const QString player = findPlayer();

    if (player.isEmpty())
    {
        log("Cannot find loom_player next to the editor.", true);
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
