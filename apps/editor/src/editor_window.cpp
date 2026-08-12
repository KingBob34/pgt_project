#include "editor_window.h"

#include <QAction>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QListWidget>
#include <QMenuBar>

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

EditorWindow::EditorWindow()
{
    loom::registerBuiltinNodes(catalog);

    setWindowTitle("Loom Editor");
    resize(1400, 900);

    buildCanvas();
    buildMenus();
    buildConsole();

    newStory();
}

EditorWindow::~EditorWindow()
{
    // The view reads the scene and the model, both of which are members and so
    // are destroyed before QMainWindow deletes its children.
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

    QAction* save = file->addAction("&Save", this, &EditorWindow::saveStory);
    save->setShortcut(QKeySequence::Save);

    QAction* saveAs = file->addAction("Save &As...", this, &EditorWindow::saveStoryAs);
    saveAs->setShortcut(QKeySequence::SaveAs);

    file->addSeparator();

    QAction* quit = file->addAction("&Quit", this, &QWidget::close);
    quit->setShortcut(QKeySequence::Quit);
}

void EditorWindow::buildConsole()
{
    console = new QListWidget;
    console->setMinimumHeight(140);

    QDockWidget* dock = new QDockWidget("Console", this);
    dock->setWidget(console);

    addDockWidget(Qt::BottomDockWidgetArea, dock);
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
    project.graphs.emplace_back();

    editing = 0;

    scene->undoStack().clear();
    console->clear();

    setStoryPath(QString());
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

    document->open(project.graphs[editing]);

    scene->undoStack().clear();

    setStoryPath(path);

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

bool EditorWindow::writeStory(const QString& path)
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

