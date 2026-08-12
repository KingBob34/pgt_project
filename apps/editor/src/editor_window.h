#ifndef LOOM_EDITOR_EDITOR_WINDOW_H
#define LOOM_EDITOR_EDITOR_WINDOW_H
#include <cstddef>
#include <memory>
#include <string>

#include <QMainWindow>
#include <QString>

#include <QtNodes/NodeDelegateModelRegistry>

#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

class GraphDocument;
class GraphModel;
class GraphScene;
class GraphView;
class QListWidget;
class QListWidgetItem;

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow();
    ~EditorWindow() override;

    void openStory(const QString& path);

private:
    void buildMenus();
    void buildCanvas();
    void buildConsole();
    void buildScenes();

    void newStory();
    void chooseStory();
    bool saveStory();
    bool saveStoryAs();
    bool writeStory(const QString& path);

    void setStoryPath(const QString& path);
    void log(const QString& text, bool fault = false);
    void report(const loom::Diagnostics& diagnostics);

    void refreshScenes();
    void showScene(const loom::Graph& graph);
    void switchScene(int index);
    void addScene();
    void removeScene();
    void renameScene(QListWidgetItem* item);

    std::string uniqueSceneName(const std::string& wanted) const;
    void reportSceneReferences(const std::string& name);

    loom::NodeCatalog catalog;

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry;
    std::unique_ptr<GraphModel>                         model;
    std::unique_ptr<GraphDocument>                      document;

    GraphScene* scene = nullptr;
    GraphView*  view  = nullptr;

    QListWidget* console = nullptr;
    QListWidget* scenes = nullptr;

    // Set while the scene list is being refilled, so its own signals do not
    // read back as the author clicking about.
    bool rebuilding = false;

    loom::Project project;
    std::size_t   editing = 0;

    QString storyPath;
};

#endif //LOOM_EDITOR_EDITOR_WINDOW_H
