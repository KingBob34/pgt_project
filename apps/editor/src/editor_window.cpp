#include "editor_window.h"

#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStringList>

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

void EditorWindow::newStory()
{
    document->reset();

    project = loom::Project();
    project.entry = document->name();
    project.graphs.emplace_back();

    editing = 0;

    scene->undoStack().clear();

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
        QMessageBox::warning(this, "Loom Editor", "Cannot read " + path);
        return;
    }

    loom::Value parsed;
    std::string error;

    if (!loom::parseJson(file.readAll().toStdString(), parsed, error))
    {
        QMessageBox::warning(this, "Loom Editor", QString::fromStdString(error));
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
        showDiagnostics(diagnostics, "This story was not opened.");
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

    if (!diagnostics.all().empty()) showDiagnostics(diagnostics, "The story was opened.");
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
        QMessageBox::warning(this, "Loom Editor", "Cannot write " + path);
        return false;
    }

    file.write(QByteArray::fromStdString(loom::writeJson(loom::writeProject(project))));
    file.close();

    setStoryPath(path);

    // Unfinished work is saved all the same, and every fault in it is listed.
    loom::Diagnostics diagnostics;
    loom::validate(project, catalog, diagnostics);

    if (!diagnostics.all().empty()) showDiagnostics(diagnostics, "The story was saved.");

    return true;
}

void EditorWindow::setStoryPath(const QString& path)
{
    storyPath = path;

    const QString shown = path.isEmpty() ? QString("Untitled") : QFileInfo(path).fileName();

    setWindowTitle(shown + " - Loom Editor");
}

void EditorWindow::showDiagnostics(const loom::Diagnostics& diagnostics, const QString& heading)
{
    QStringList lines;

    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        QString line = entry.severity == loom::Severity::Error ? "Error" : "Warning";

        if (entry.node != 0)    line += " at node " + QString::number(entry.node);
        if (!entry.pin.empty()) line += ", pin " + QString::fromStdString(entry.pin);

        lines << line + ": " + QString::fromStdString(entry.message);
    }

    if (lines.isEmpty()) lines << "The file could not be read at all.";

    QMessageBox box(this);
    box.setWindowTitle("Loom Editor");
    box.setIcon(diagnostics.hasErrors() ? QMessageBox::Warning : QMessageBox::Information);
    box.setText(heading);
    box.setInformativeText(QString::number(lines.size()) + " reported. Open Details for the list.");
    box.setDetailedText(lines.join('\n'));
    box.exec();
}
