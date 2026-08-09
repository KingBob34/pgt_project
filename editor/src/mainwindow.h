#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "editor_scene.h"
#include <memory>
#include <QMainWindow>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/NodeDelegateModelRegistry>

class EditorView;
class QAction;
class QDockWidget;

// The application shell: menus, toolbar, docked panels, etc.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
private:
    void createGraph();
    void createActions();
    void createMenus();
    void createToolBar();
    void createPanels();
    QDockWidget* addPanel(const QString& title, Qt::DockWidgetArea area);

    // The graph
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry;
    std::unique_ptr<QtNodes::DataFlowGraphModel> model;
    std::unique_ptr<EditorScene> scene;
    QtNodes::NodeId startNodeId = 0;


    // Owned by QMainWindow as the central widget
    EditorView* view = nullptr;

    // One object per command, shared by its menu item, toolbar button and shortcut
    QAction* newAction = nullptr;
    QAction* openAction = nullptr;
    QAction* saveAction = nullptr;
    QAction* exitAction = nullptr;
    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;

    // Panels
    QDockWidget* variablesPanel = nullptr;
    QDockWidget* detailsPanel = nullptr;
    QDockWidget* assetsPanel = nullptr;
};

#endif //MAINWINDOW_H
