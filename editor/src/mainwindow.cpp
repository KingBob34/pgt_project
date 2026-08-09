#include "mainwindow.h"
#include "editor_view.h"
#include "nodes/editor_boolean_node.h"
#include "nodes/editor_branch_node.h"
#include "nodes/editor_choice_node.h"
#include "nodes/editor_compare_node.h"
#include "nodes/editor_condition_node.h"
#include "nodes/editor_end_node.h"
#include "nodes/editor_narrative_node.h"
#include "nodes/editor_node_geometry.h"
#include "nodes/editor_start_node.h"
#include "nodes/editor_value_node.h"

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFileSystemModel>
#include <QKeySequence>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QPointF>
#include <QSize>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    createGraph();
    createActions();
    createPanels();   // before createMenus(): the View menu lists the panels
    createMenus();
    createToolBar();
}

MainWindow::~MainWindow()
{
    delete takeCentralWidget();
}

void MainWindow::createGraph()
{
    // Node types available in the right-click menu
    registry = std::make_shared<QtNodes::NodeDelegateModelRegistry>();
    registry->registerModel<StartNode>("Flow");
    registry->registerModel<EndNode>("Flow");
    registry->registerModel<NarrativeNode>("Story");
    registry->registerModel<ChoiceNode>("Story");
    registry->registerModel<ConditionNode>("Logic");
    registry->registerModel<BranchNode>("Logic");

    registry->registerModel<EqualNode>("Compare");
    registry->registerModel<NotEqualNode>("Compare");
    registry->registerModel<LessNode>("Compare");
    registry->registerModel<LessEqualNode>("Compare");
    registry->registerModel<GreaterNode>("Compare");
    registry->registerModel<GreaterEqualNode>("Compare");
    registry->registerModel<ContainsNode>("Compare");

    registry->registerModel<GetVarNode>("Value");
    registry->registerModel<GetListNode>("Value");
    registry->registerModel<LiteralNode>("Value");

    registry->registerModel<AndNode>("Boolean");
    registry->registerModel<OrNode>("Boolean");
    registry->registerModel<NotNode>("Boolean");

    model = std::make_unique<QtNodes::DataFlowGraphModel>(registry);
    scene = std::make_unique<EditorScene>(*model);
    scene->setNodeGeometry(std::make_unique<EditorNodeGeometry>(*model));

    view = new EditorView(scene.get());
    setCentralWidget(view);

    // Every graph owns exactly one start node
    startNodeId = model->addNode("start");
    model->setNodeData(startNodeId, QtNodes::NodeRole::Position, QPointF(-300, 0));
    view->protectFromDeletion(startNodeId);

}

void MainWindow::createActions()
{
    newAction = new QAction("New", this);
    newAction->setShortcut(QKeySequence::New);

    openAction = new QAction("Open...", this);
    openAction->setShortcut(QKeySequence::Open);

    saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);

    exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);

    redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
}

QDockWidget* MainWindow::addPanel(const QString& title, Qt::DockWidgetArea area)
{
    auto* dock = new QDockWidget(title, this);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(area, dock);
    return dock;
}

void MainWindow::createPanels()
{
    variablesPanel = addPanel("Variables", Qt::LeftDockWidgetArea);
    detailsPanel = addPanel("Details", Qt::RightDockWidgetArea);
    assetsPanel = addPanel("Assets", Qt::BottomDockWidgetArea);

    // Asset browser
    const QString rootPath = QDir::homePath();

    auto* files = new QFileSystemModel(assetsPanel);
    files->setRootPath(rootPath);

    auto* list = new QListView(assetsPanel);
    list->setModel(files);
    list->setRootIndex(files->index(rootPath));
    list->setViewMode(QListView::IconMode);
    list->setResizeMode(QListView::Adjust);
    list->setMovement(QListView::Static);
    list->setIconSize(QSize(48, 48));
    list->setGridSize(QSize(110, 90));
    list->setWordWrap(true);
    list->setUniformItemSizes(true);

    assetsPanel->setWidget(list);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(newAction);
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);

    QMenu* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction(variablesPanel->toggleViewAction());
    viewMenu->addAction(detailsPanel->toggleViewAction());
    viewMenu->addAction(assetsPanel->toggleViewAction());
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar("Main");
    toolBar->setMovable(false);

    // The same QAction objects the menus hold
    toolBar->addAction(newAction);
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addSeparator();
    toolBar->addAction(undoAction);
    toolBar->addAction(redoAction);
}
