#include "mainwindow.h"
#include "canvasview.h"
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QDockWidget>
#include <QTreeView>
#include <QListView>
#include <QFileSystemModel>
#include <QDir>

#include "canvasview.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    createMenus();
    createToolBar();
    createFileBrowser();
    setCentralWidget(new CanvasView(this));
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New");
    fileMenu->addAction("Open...");
    fileMenu->addAction("Save");
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    QMenu* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction("Undo");
    editMenu->addAction("Redo");

    menuBar()->addMenu("View");
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = new QToolBar("Main", this);
    addToolBar(Qt::LeftToolBarArea, toolBar);
    toolBar->addAction("New");
    toolBar->addAction("Open");
    toolBar->addAction("Save");
}

void MainWindow::createFileBrowser()
{
    QDockWidget* dock = new QDockWidget("Files", this);
    QFileSystemModel* model = new QFileSystemModel(dock);
    QString rootPath = "C:/Users/KingBob/Desktop";
    model->setRootPath(rootPath);

    QListView* list = new QListView(dock);
    list->setModel(model);
    list->setRootIndex(model->index(rootPath));
    list->setViewMode(QListView::IconMode);
    list->setResizeMode(QListView::Adjust);
    list->setMovement(QListView::Static);
    list->setIconSize(QSize(48, 48));
    list->setGridSize(QSize(110, 90));
    list->setWordWrap(true);
    list->setUniformItemSizes(true);

    dock->setWidget(list);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}
