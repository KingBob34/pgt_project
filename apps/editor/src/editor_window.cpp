#include "editor_window.h"

#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeySequence>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QProcess>
#include <QProxyStyle>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>

#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include "connection_painter.h"
#include "details_panel.h"
#include "graph_document.h"
#include "graph_model.h"
#include "graph_scene.h"
#include "graph_view.h"
#include "inline_edit.h"
#include "node_adaptor.h"
#include "node_geometry.h"
#include "node_painter.h"
#include "node_palette.h"
#include "playtest_panel.h"
#include "value_tree.h"

#include "loom/qt/convert.h"
#include "loom/graph/validate.h"
#include "loom/nodes/builtin.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

using loom::qt::toQt;

namespace
{
    // A tab bar slides a tab into its new place but writes the label straight
    // at where it is going, so the name arrives before the box it belongs to.
    // Taking the slide away keeps the two together.
    class InstantTabs : public QProxyStyle
    {
    public:
        int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                      QStyleHintReturn* data) const override
        {
            if (hint == SH_Widget_Animation_Duration) return 0;

            return QProxyStyle::styleHint(hint, option, widget, data);
        }
    };

    // The narrowest the left column is worth being: both panels in it read
    // as prose.
    constexpr int kLeastSideWidth = 320;

    // What the variable panel's three columns need side by side, and the room
    // it opens with so a value has somewhere to be read.
    constexpr int kLeastPanelWidth = 320;
    constexpr int kPanelWidth = 360;

    // The gap the style sheet leaves after each tab. tabRect() counts it as
    // part of the tab, so the rename box has to give it back.
    constexpr int kSceneTabGap = 10;

    // Tabs sit against one another by default, which reads as one long bar
    // rather than as a scene each. A background of their own is what makes
    // them separate, and the margin is the gap between them.
    QString sceneStripStyle()
    {
        return QString("QTabBar { border: none; }"
                       "QTabBar::tab {"
                       "  background: #3a3a3f;"
                       "  color: #d0d0d0;"
                       "  border: 1px solid #4e4e54;"
                       "  border-radius: 5px;"
                       "  padding: 11px 22px;"
                       "  margin-right: %1px;"
                       "  min-width: 56px;"
                       "}"
                       "QTabBar::tab:selected {"
                       "  background: #3d4a5a; border-color: #8aa0bb; color: #ffffff;"
                       "}"
                       "QTabBar::tab:!selected:hover { background: #46464c; }").arg(kSceneTabGap);
    }

    // Tall enough for a tab, whether or not there is one in it yet.
    constexpr int kSceneRowHeight = 44;

    // How far the rename box sits inside the tab it is typed on.
    constexpr int kSceneBoxInset = 6;

    // The scene the story begins in.
    const QColor kEntryScene(0xd8, 0xbc, 0x6a);

    // Long enough for windeployqt to walk a fresh binary, short enough that a
    // tool which never returns does not hold the editor for ever.
    constexpr int kDeployTimeout = 120000;

    // Where an exported game keeps Qt's plugins. Gathering them under one name
    // is the difference between a folder of four things and a folder of eleven.
    const char* const kRuntimeFolder = "runtime";

    // Plugins for things a story never does. Left out, Qt6Network stops being
    // pulled in with them.
    const char* const kUnusedPlugins = "networkinformation,tls,generic,iconengines";

    // Where a file dialog opens when the story it is about has no path yet.
    QString storyFolder(const QString& current)
    {
        return current.isEmpty() ? QString(LOOM_STORIES_DIR) : current;
    }

    // Overwriting, because the same folder may be exported into twice.
    bool copyOver(const QString& from, const QString& into)
    {
        QFile::remove(into);

        return QFile::copy(from, into);
    }

    // A folder and everything under it. Plugin trees are one level deep, but
    // nothing here depends on that.
    bool copyTree(const QString& from, const QString& into)
    {
        const QDir source(from);

        if (!source.exists() || !QDir().mkpath(into)) return false;

        const QFileInfoList entries =
            source.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo& entry : entries)
        {
            const QString target = into + "/" + entry.fileName();

            if (entry.isDir())
            {
                if (!copyTree(entry.absoluteFilePath(), target)) return false;
                continue;
            }

            if (!copyOver(entry.absoluteFilePath(), target)) return false;
        }

        return true;
    }

    // The libraries the editor is itself standing on. A packaged engine has
    // them beside it and hands the game the very same ones; one running from a
    // build tree has none, because Qt is on its PATH instead.
    bool copyEngineRuntime(const QString& into)
    {
        const QDir beside(QCoreApplication::applicationDirPath());
        const QStringList libraries = beside.entryList({ "*.dll" }, QDir::Files);

        if (libraries.isEmpty() || !beside.exists(kRuntimeFolder)) return false;

        for (const QString& library : libraries)
        {
            if (!copyOver(beside.filePath(library), into + "/" + library)) return false;
        }

        return copyTree(beside.filePath(kRuntimeFolder), into + "/" + kRuntimeFolder);
    }

    // The tool that walks a binary and fetches what it imports. Where it lives
    // is settled when the editor is built, so it is only there on the machine
    // that built it. Answers with what went wrong, or with nothing.
    QString fetchQtLibraries(const QString& binary)
    {
        QProcess deploy;

        deploy.start(LOOM_WINDEPLOYQT,
                     { "--no-translations", "--no-system-d3d-compiler", "--no-opengl-sw",
                       "--skip-plugin-types", kUnusedPlugins,
                       "--plugindir", QFileInfo(binary).path() + "/" + kRuntimeFolder,
                       binary });

        if (!deploy.waitForFinished(kDeployTimeout)) return "windeployqt did not finish";

        if (deploy.exitCode() != 0)
        {
            return QString::fromLocal8Bit(deploy.readAllStandardError()).trimmed();
        }

        return QString();
    }

    // Qt looks beside the exe for its plugins unless a qt.conf sends it
    // somewhere else.
    bool writeQtConf(const QString& into)
    {
        QFile file(into + "/qt.conf");

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

        file.write(QByteArray("[Paths]\nPlugins = ") + kRuntimeFolder + "\n");

        return true;
    }

    // In a package the game sits under runtime/ with the rest of the engine's
    // own parts, so that the only thing at the top of the folder an author can
    // double-click is the editor. In a build tree each target has its own
    // directory instead.
    QString findGame()
    {
        const QString name = "LoomGame.exe";
        const QString here = QCoreApplication::applicationDirPath();

        for (const QString& candidate : { here + "/" + kRuntimeFolder + "/" + name,
                                          here + "/" + name,
                                          here + "/../player/" + name })
        {
            if (QFileInfo::exists(candidate)) return QFileInfo(candidate).absoluteFilePath();
        }

        return QString();
    }

    // A green play triangle. The stock media icon is dark on a dark toolbar.
    // The badge marks the one that starts at the chosen node instead.
    QIcon playIcon(bool fromHere)
    {
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(104, 198, 104));

        QPolygonF triangle;
        triangle << QPointF(5.0, 3.0) << QPointF(18.0, 11.0) << QPointF(5.0, 19.0);

        painter.drawPolygon(triangle);

        if (fromHere)
        {
            // A card, because that is what the author is pointing at, in the
            // colour the canvas rings the node they have picked out with.
            const QRectF badge(10.0, 12.0, 13.0, 10.0);

            // Punched out of the triangle first, so the badge keeps its edge
            // wherever the two overlap.
            painter.setBrush(Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.drawRoundedRect(badge.adjusted(-1.5, -1.5, 1.5, 1.5), 3.5, 3.5);

            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.setBrush(palette::border(true));
            painter.drawRoundedRect(badge, 2.5, 2.5);
        }

        painter.end();

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

    // Shown, then handed the keyboard on the next turn of the event loop. The
    // canvas embeds real widgets in the scene, and one of those coming up can
    // take the focus off the window that owns it.
    QTimer::singleShot(0, this, [this]
    {
        raise();
        activateWindow();
    });
}

EditorWindow::~EditorWindow()
{
    // The view reads the scene and the model, and both are destroyed before it.
    delete takeCentralWidget();
}

void EditorWindow::buildCanvas()
{
    registry = makeRegistry(catalog, variableSpecs, sceneNames, [this] { return model.get(); });
    model    = std::make_unique<GraphModel>(registry);

    scene = new GraphScene(*model, catalog, this);
    scene->setNodeGeometry(std::make_unique<NodeGeometry>(*model));
    scene->setNodePainter(std::make_unique<NodePainter>(faults));
    scene->setConnectionPainter(std::make_unique<ConnectionPainter>(faults));

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

    file->addAction("&Export Game...", this, &EditorWindow::exportGame);

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

    edit->addSeparator();

    // The canvas keeps these, because it is the canvas that has a selection
    // and a place to paste into. QtNodes hangs its own copies on the view, on
    // the same keys, and those are taken off there so the keys are not claimed
    // twice: a key claimed twice is one Qt answers by doing nothing.
    const auto onCanvas = [this](QAction* action, QKeySequence key)
    {
        // Scoped to the canvas rather than to the window, so that Ctrl+C in a
        // panel on the side still copies the text the author selected there.
        action->setShortcut(key);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

        view->addAction(action);
    };

    onCanvas(edit->addAction("Cu&t", view, &GraphView::onCutSelectedObjects),
             QKeySequence::Cut);
    onCanvas(edit->addAction("&Copy", view, &GraphView::onCopySelectedObjects),
             QKeySequence::Copy);
    onCanvas(edit->addAction("&Paste", view, &GraphView::onPasteObjects),
             QKeySequence::Paste);
    onCanvas(edit->addAction("&Duplicate", view, &GraphView::onDuplicateSelectedObjects),
             QKeySequence(Qt::CTRL | Qt::Key_D));

    edit->addSeparator();

    // No shortcut of its own: Backspace is read by the canvas, which knows
    // whether a pin is being typed into and has to keep its own key.
    edit->addAction("De&lete", view, &GraphView::onDeleteSelectedObjects);

    // Filled in by buildDocks, which is where the panels are made.
    panelMenu = menuBar()->addMenu("&View");

    QMenu* debug = menuBar()->addMenu("&Debug");

    playAction = debug->addAction("&Play", this, &EditorWindow::playStory);
    playAction->setShortcut(Qt::Key_F5);

    playHereAction = debug->addAction("Play From &Here", this, &EditorWindow::playStoryHere);
    playHereAction->setShortcut(Qt::SHIFT | Qt::Key_F5);

    debug->addSeparator();

    debug->addAction("&Clear Console", this, &EditorWindow::clearConsole);
}

void EditorWindow::buildToolBar()
{
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    playAction->setIcon(playIcon(false));
    playHereAction->setIcon(playIcon(true));

    playAction->setToolTip("Play from the start of the story");
    playHereAction->setToolTip("Play from the selected node");

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
    bar->addAction(playHereAction);
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
    scenes = new QTabBar;
    scenes->setDrawBase(false);
    scenes->setElideMode(Qt::ElideNone);
    scenes->setStyleSheet(sceneStripStyle());

    InstantTabs* instant = new InstantTabs;
    instant->setParent(scenes);

    scenes->setStyle(instant);
    scenes->setContextMenuPolicy(Qt::CustomContextMenu);

    // Without this a lone scene is stretched across the whole bar.
    scenes->setExpanding(false);

    // A story with one scene still has to show it. The bar is laid out before
    // the first scene is put in it, and an empty one asks for no height at
    // all: it would be given none, and the tab added afterwards would be
    // drawn inside nothing.
    scenes->setAutoHide(false);
    scenes->setMinimumHeight(kSceneRowHeight);

    // Dragged into whatever order the author likes to read them in.
    scenes->setMovable(true);

    connect(scenes, &QTabBar::currentChanged, this, &EditorWindow::switchScene);
    connect(scenes, &QTabBar::tabBarDoubleClicked, this, &EditorWindow::renameScene);
    connect(scenes, &QTabBar::tabMoved, this, &EditorWindow::reorderScenes);

    connect(scenes, &QTabBar::customContextMenuRequested, this, [this](const QPoint& at)
    {
        const int tab = scenes->tabAt(at);
        if (tab < 0) return;

        QMenu menu;
        QAction* start = menu.addAction("Start Here");
        QAction* drop = menu.addAction("Delete Scene");

        drop->setEnabled(project.graphs.size() > 1);

        const QAction* chosen = menu.exec(scenes->mapToGlobal(at));
        if (chosen == nullptr) return;

        scenes->setCurrentIndex(tab);

        if (chosen == drop)
        {
            removeScene();
            return;
        }

        project.entry = project.graphs[static_cast<std::size_t>(tab)].name;
        refreshScenes();
    });

    // Only the one button: a scene is made often and deleted once in a while,
    // so deleting lives in the menu on the scene itself, as a variable does.
    QPushButton* add = new QPushButton("+");
    add->setFixedSize(34, 34);
    add->setToolTip("New scene");

    connect(add, &QPushButton::clicked, this, &EditorWindow::addScene);

    QWidget* strip = new QWidget;

    QHBoxLayout* row = new QHBoxLayout(strip);
    row->setContentsMargins(10, 18, 10, 18);
    row->setSpacing(10);
    row->addWidget(add);
    row->addWidget(scenes, 1);

    return strip;
}

QWidget* EditorWindow::buildPanel()
{
    panel = new QTabWidget;
    panel->setDocumentMode(true);

    details = new DetailsPanel;
    values  = new ValueTree;

    // Off the panel until a node asks for it, and parented so that it is never
    // a widget without one, which in Qt means a window.
    details->setParent(panel);
    details->hide();

    connect(values, &ValueTree::changed, this, &EditorWindow::syncVariableNames);
    connect(values, &ValueTree::renamed, this, &EditorWindow::renameVariable);
    connect(values, &ValueTree::removed, this, &EditorWindow::reportVariableUses);

    panel->addTab(values, "Global Variables");

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

    refreshNodeEditors();
}

void EditorWindow::refreshNodeEditors()
{
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
              toQt(graph.name) + "'",
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
              toQt(graph.name) + "'",
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

    playtest = new PlaytestPanel(catalog);

    connect(playtest, &PlaytestPanel::faulted, this,
            [this](const QString& detail, const QString& graph, loom::NodeId node)
            {
                logAt(detail, graph.toStdString(), node, true);
            });

    playtestDock = new QDockWidget("Playtest", this);
    playtestDock->setWidget(playtest);
    addDockWidget(Qt::LeftDockWidgetArea, playtestDock);

    QDockWidget* output = new QDockWidget("Console", this);
    output->setWidget(buildConsole());
    addDockWidget(Qt::LeftDockWidgetArea, output);

    splitDockWidget(playtestDock, output, Qt::Vertical);

    QDockWidget* strip = new QDockWidget("Scenes", this);
    strip->setWidget(buildScenes());
    addDockWidget(Qt::BottomDockWidgetArea, strip);

    QDockWidget* inspector = new QDockWidget("Inspector", this);
    inspector->setWidget(buildPanel());

    addDockWidget(Qt::RightDockWidgetArea, inspector);

    resizeDocks({ playtestDock, output }, { 420, 300 }, Qt::Vertical);
    // The playtest text and the console both read as prose, so neither is
    // worth having narrower than a column of it.
    playtestDock->setMinimumWidth(kLeastSideWidth);
    output->setMinimumWidth(kLeastSideWidth);

    // Narrower than this the variable panel's three columns no longer fit
    // side by side, and the tree starts carrying the name column off its own
    // left edge.
    inspector->setMinimumWidth(kLeastPanelWidth);

    resizeDocks({ playtestDock, output, inspector }, { kLeastSideWidth, kLeastSideWidth, kPanelWidth },
                Qt::Horizontal);

    // Closing a dock hides it, and these are the only way to ask for it back.
    panelMenu->addAction(playtestDock->toggleViewAction());
    panelMenu->addAction(output->toggleViewAction());
    panelMenu->addAction(inspector->toggleViewAction());
    panelMenu->addAction(strip->toggleViewAction());

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

    showDetails(only != nullptr);
}

void EditorWindow::showDetails(bool on)
{
    const int at = panel->indexOf(details);

    if (on)
    {
        if (at < 0) panel->addTab(details, "Node Details");

        panel->setCurrentWidget(details);
        return;
    }

    if (at < 0) return;

    panel->removeTab(at);

    // removeTab hands the page back with no parent at all, and for that one
    // moment it is a top level window flashing over the canvas.
    details->setParent(panel);
    details->hide();
}

void EditorWindow::refreshScenes()
{
    closeSceneBox();

    rebuilding = true;

    while (scenes->count() > 0) scenes->removeTab(0);

    // The nodes read this while they build their menus, so it is filled in the
    // same pass that draws the tabs and can never disagree with them.
    const std::vector<std::string> before = std::move(sceneNames);

    sceneNames.clear();

    for (const loom::Graph& graph : project.graphs)
    {
        sceneNames.push_back(graph.name);

        const int tab = scenes->addTab(toQt(graph.name));

        // Where the story begins. A tab bar has one font for all of its tabs,
        // so the entry is marked in colour rather than in weight.
        if (graph.name == project.entry) scenes->setTabTextColor(tab, kEntryScene);

        scenes->setTabToolTip(tab, "Double click to rename");
    }

    scenes->setCurrentIndex(static_cast<int>(editing));

    rebuilding = false;

    if (sceneNames != before) refreshNodeEditors();
}

void EditorWindow::showScene(const loom::Graph& graph)
{
    // Painting is off while the canvas empties and refills.
    view->setUpdatesEnabled(false);

    document->open(graph);

    view->setUpdatesEnabled(true);

    scene->undoStack().clear();

    // Where a scene begins is where the author wants to be looking. Left to
    // itself the view settles on the middle of the rectangle the nodes span,
    // which moves whenever a node changes size.
    for (QtNodes::NodeId node : model->allNodeIds())
    {
        const NodeAdaptor* adaptor = adaptorFor(*model, node);

        if (adaptor == nullptr || !adaptor->nodeType().isEntryPoint()) continue;

        if (QtNodes::NodeGraphicsObject* drawn = scene->nodeGraphicsObject(node))
        {
            view->centerOn(drawn);
        }

        break;
    }

    syncDetails();
}

void EditorWindow::switchScene(int index)
{
    if (rebuilding || index < 0) return;

    const std::size_t wanted = static_cast<std::size_t>(index);

    if (wanted == editing || wanted >= project.graphs.size()) return;

    project.graphs[editing] = document->graph();

    editing = wanted;

    // The marks name node ids in the scene that raised them, and ids only mean
    // something inside one scene.
    faults.clear();

    showScene(project.graphs[editing]);
}

void EditorWindow::reorderScenes(int from, int to)
{
    closeSceneBox();

    const std::size_t count = project.graphs.size();

    if (rebuilding || from < 0 || to < 0) return;
    if (std::size_t(from) >= count || std::size_t(to) >= count) return;

    // The tab bar has already moved the tab; this brings the story in line
    // with it. Nothing but the reading order changes.
    const loom::Graph carried = project.graphs[std::size_t(from)];

    project.graphs.erase(project.graphs.begin() + from);
    project.graphs.insert(project.graphs.begin() + to, carried);

    if (std::size_t(from) == editing)                        editing = std::size_t(to);
    else if (std::size_t(from) < editing && std::size_t(to) >= editing) --editing;
    else if (std::size_t(from) > editing && std::size_t(to) <= editing) ++editing;

    // Guarded, or the index landing back on the scene already open would be
    // read as a request to open it again.
    rebuilding = true;
    scenes->setCurrentIndex(static_cast<int>(editing));
    rebuilding = false;
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

    log("Removed the scene '" + toQt(gone) + "'");

    reportSceneReferences(gone);
}

void EditorWindow::renameScene(int tab)
{
    if (rebuilding || tab < 0) return;

    const std::size_t index = static_cast<std::size_t>(tab);

    if (index >= project.graphs.size()) return;

    const std::string was = project.graphs[index].name;

    // Typed on the tab itself rather than in a dialog. The tab's own label
    // comes off while the box is up, or the two names sit on one another.
    scenes->setTabText(tab, QString());

    InlineEdit* box = new InlineEdit(toQt(was),
                                     [this, was](const QString& typed)
                                     {
                                         takeSceneName(was, typed.toStdString());
                                     },
                                     scenes);

    connect(box, &InlineEdit::finished, box, &QObject::deleteLater);

    const QRect over = scenes->tabRect(tab).adjusted(kSceneBoxInset, kSceneBoxInset,
                                                     -(kSceneBoxInset + kSceneTabGap),
                                                     -kSceneBoxInset);

    // Fixed rather than set: a line edit asks for a width of its own, and a
    // plain setGeometry is answered with that one instead.
    box->setFixedSize(over.size());
    box->move(over.topLeft());

    box->show();
    box->setFocus();
    box->selectAll();

    sceneBox = box;
}

void EditorWindow::closeSceneBox()
{
    // A tab bar takes no focus from a click, so dragging the tabs about leaves
    // the box sitting over whichever tab has arrived underneath it.
    if (!sceneBox.isNull()) sceneBox->clearFocus();
}

void EditorWindow::takeSceneName(const std::string& was, const std::string& typed)
{
    std::size_t index = project.graphs.size();

    for (std::size_t at = 0; at < project.graphs.size(); ++at)
    {
        if (project.graphs[at].name == was) index = at;
    }

    // Either way the tabs are rebuilt, which is what puts back the label that
    // was taken off while the box was up.
    if (index == project.graphs.size() || typed.empty() || typed == was)
    {
        refreshScenes();
        return;
    }

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
            const loom::NodeType* type = catalog.find(node.type);
            if (type == nullptr) continue;

            // Only the pins that pick a scene out of a list. Any other string
            // that happens to read the same is not a reference to anything.
            for (const loom::PinSpec& pin : type->pins(node.extraPins))
            {
                if (pin.type != loom::PinType::SceneName) continue;

                const auto stored = node.pinValues.find(pin.name);

                if (stored == node.pinValues.end()) continue;
                if (loom::asString(stored->second) != name) continue;

                logAt("Warning in scene '" + toQt(graph.name) + "' at " +
                      nodeLabel(graph.name, node.id) + ", pin " +
                      toQt(pin.name) + ": still names the scene '" +
                      toQt(name) + "'",
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

    line->setData(Qt::UserRole, toQt(graph));
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

            if (type != nullptr) return toQt(type->displayName()) + number;
        }
    }

    return "node" + number;
}

void EditorWindow::revealNode(QListWidgetItem* line)
{
    const QVariant target = line->data(Qt::UserRole + 1);

    if (!target.isValid()) return;

    focusNode(line->data(Qt::UserRole).toString().toStdString(), target.toInt());
}

bool EditorWindow::focusNode(const std::string& graph, loom::NodeId node)
{
    if (!graph.empty() && graph != project.graphs[editing].name)
    {
        std::size_t index = project.graphs.size();

        for (std::size_t at = 0; at < project.graphs.size(); ++at)
        {
            if (project.graphs[at].name == graph) index = at;
        }

        if (index == project.graphs.size())
        {
            log("There is no longer a scene called '" + toQt(graph) + "'.", true);
            return false;
        }

        scenes->setCurrentIndex(static_cast<int>(index));
    }

    QtNodes::NodeGraphicsObject* object =
        scene->nodeGraphicsObject(static_cast<QtNodes::NodeId>(node));

    if (object == nullptr)
    {
        log("Node #" + QString::number(node) + " is no longer on the canvas.", true);
        return false;
    }

    scene->clearSelection();
    object->setSelected(true);

    view->centerOn(object);

    return true;
}

void EditorWindow::report(const loom::Diagnostics& diagnostics)
{
    faults.clear();

    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        const bool fault = entry.severity == loom::Severity::Error;

        // Only the scene on screen can be coloured, and only errors are worth
        // colouring: a warning is a story that is merely unfinished.
        if (fault && entry.graph == project.graphs[editing].name)
        {
            faults.add(entry.node, entry.pin);
        }

        QString line = fault ? "Error" : "Warning";

        if (entry.node != 0)    line += " at " + nodeLabel(entry.graph, entry.node);
        if (!entry.pin.empty()) line += ", pin " + toQt(entry.pin);

        line += ": " + toQt(entry.message);

        if (entry.node == 0) log(line, fault);
        else                 logAt(line, entry.graph, entry.node, fault);
    }

    scene->update();
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

    saved = snapshot();
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
        log(toQt(error), true);
        return;
    }

    loom::Project     opened;
    loom::Diagnostics diagnostics;

    const bool read = loom::readProject(parsed, catalog, opened, diagnostics);

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

    saved = snapshot();
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

void EditorWindow::gatherProject()
{
    project.graphs[editing] = document->graph();
    project.variables = values->variables();
}

bool EditorWindow::writeProjectTo(const QString& path)
{
    gatherProject();

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

    saved = snapshot();

    return true;
}

std::string EditorWindow::snapshot()
{
    gatherProject();

    return loom::writeJson(loom::writeProject(project));
}

bool EditorWindow::mayDiscard()
{
    if (snapshot() == saved) return true;

    QMessageBox asking(this);
    asking.setIcon(QMessageBox::Warning);
    asking.setWindowTitle("Loom");
    asking.setText("This story has changes that have not been saved.");
    asking.setInformativeText("Save them before closing?");
    asking.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    asking.setDefaultButton(QMessageBox::Save);

    const int answer = asking.exec();

    // Saving may still be called off at the file dialog, and then the window
    // stays open too.
    if (answer == QMessageBox::Save) return saveStory();

    return answer == QMessageBox::Discard;
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (mayDiscard()) event->accept();
    else               event->ignore();
}

void EditorWindow::setStoryPath(const QString& path)
{
    storyPath = path;

    const QString shown = path.isEmpty() ? QString("Untitled") : QFileInfo(path).fileName();

    setWindowTitle(shown + " - Loom Editor");
}

void EditorWindow::playStory()
{
    gatherProject();

    playtestDock->show();
    playtestDock->raise();

    playtest->play(project);

    log("Playing " + toQt(project.entry));
}

void EditorWindow::playStoryHere()
{
    QtNodes::NodeGraphicsObject* only = nullptr;

    for (QGraphicsItem* item : scene->selectedItems())
    {
        QtNodes::NodeGraphicsObject* object =
            qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item);

        if (object == nullptr) continue;

        // A run begins at one node, so a wider selection names none.
        if (only != nullptr)
        {
            only = nullptr;
            break;
        }

        only = object;
    }

    if (only == nullptr)
    {
        log("Select the node to start from first.", true);
        return;
    }

    const loom::NodeId from = static_cast<loom::NodeId>(only->nodeId());
    const std::string sceneName = project.graphs[editing].name;

    gatherProject();

    playtestDock->show();
    playtestDock->raise();

    playtest->playFrom(project, sceneName, from);

    log("Playing from " + nodeLabel(sceneName, from));
}

void EditorWindow::exportGame()
{
    const QString game = QInputDialog::getText(this, "Export Game", "Game name")
                             .trimmed()
                             .remove(QRegularExpression("[\\\\/:*?\"<>|]"));

    if (game.isEmpty()) return;

    const QString into = QFileDialog::getExistingDirectory(this, "Export Into",
                                                           storyFolder(storyPath));
    if (into.isEmpty()) return;

    const QString source = findGame();

    if (source.isEmpty())
    {
        log("Cannot find LoomGame next to the editor.", true);
        return;
    }

    QDir folder(into + "/" + game);

    if (!folder.exists() && !QDir(into).mkpath(game))
    {
        log("Cannot make the folder " + folder.absolutePath(), true);
        return;
    }

    // Named after the game, which is how it finds its story once it is running.
    const QString binary = folder.filePath(game + ".exe");

    if (!copyOver(source, binary))
    {
        log("Cannot copy the game to " + binary, true);
        return;
    }

    if (!writeProjectTo(folder.filePath(game + ".loom"))) return;

    // A packaged engine gives the game the libraries it is running on itself,
    // which are the same ones and are already to hand. Run from a build tree
    // it has none beside it, and the Qt tool goes and fetches them.
    if (!copyEngineRuntime(folder.absolutePath()))
    {
        const QString trouble = fetchQtLibraries(binary);

        if (!trouble.isEmpty())
        {
            log("The Qt libraries were not copied; the game will not run elsewhere.", true);
            log(trouble, true);
            return;
        }
    }

    if (!writeQtConf(folder.absolutePath()))
    {
        log("Cannot write qt.conf; the game will not find its plugins.", true);
        return;
    }

    log("Exported " + game + " to " + folder.absolutePath());
}

void EditorWindow::clearConsole()
{
    console->clear();
}
