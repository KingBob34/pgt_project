#ifndef LOOM_EDITOR_EDITOR_WINDOW_H
#define LOOM_EDITOR_EDITOR_WINDOW_H
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <QMainWindow>
#include <QString>

#include <QtNodes/NodeDelegateModelRegistry>

#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

class DetailsPanel;
class GraphDocument;
class GraphModel;
class GraphScene;
class GraphView;
class ValueTree;
class QListWidget;
class QListWidgetItem;
class QAction;
class QTabWidget;

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
    void buildToolBar();
    void buildDocks();

    QWidget* buildConsole();
    QWidget* buildScenes();
    QWidget* buildPanel();

    void newStory();
    void chooseStory();
    bool saveStory();
    bool saveStoryAs();
    bool writeStory(const QString& path);
    bool writeProjectTo(const QString& path);
    void playStory();
    void clearConsole();

    void setStoryPath(const QString& path);

    QListWidgetItem* log(const QString& text, bool fault = false);

    // A console line that remembers the node it is about.
    void logAt(const QString& text, const std::string& graph, loom::NodeId node, bool fault = false);

    QString nodeLabel(const std::string& graph, loom::NodeId node) const;
    void    revealNode(QListWidgetItem* line);

    void report(const loom::Diagnostics& diagnostics);

    void syncDetails();

    // Hands the panel's names to the nodes, which offer them in their menus.
    void syncVariableNames();
    void renameVariable(const QString& before, const QString& after);
    void reportVariableUses(const QString& name);

    // Visits every pin, in every scene, that names this variable. Which pins
    // those are comes from their type, so no node type is named here.
    using VariableUse = std::function<void(std::size_t scene, loom::Graph& graph,
                                           loom::NodeInstance& node, const loom::PinSpec& pin)>;

    void forEachVariableUse(const std::string& named, const VariableUse& visit);

    void refreshScenes();
    void showScene(const loom::Graph& graph);
    void switchScene(int index);
    void addScene();
    void removeScene();
    void renameScene(QListWidgetItem* item);

    std::string uniqueSceneName(const std::string& wanted) const;
    void reportSceneReferences(const std::string& name);

    loom::NodeCatalog catalog;

    // Declared first: the registry hands a reference to it to every node.
    std::map<std::string, loom::VariableSpec> variableSpecs;

    // What the nodes are currently offering, each name with its type, to tell a
    // stale menu from a fresh one.
    std::vector<std::pair<std::string, std::string>> offeredMenu;

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry;
    std::unique_ptr<GraphModel>                         model;
    std::unique_ptr<GraphDocument>                      document;

    GraphScene* scene = nullptr;
    GraphView*  view  = nullptr;

    QListWidget* console = nullptr;
    QListWidget* scenes = nullptr;
    QTabWidget* panel = nullptr;
    DetailsPanel* details = nullptr;
    ValueTree* values = nullptr;
    QAction* saveAction = nullptr;
    QAction* playAction = nullptr;

    // True while the scene list is being refilled, so its signals can be ignored.
    bool rebuilding = false;

    loom::Project project;
    std::size_t   editing = 0;

    QString storyPath;
};

#endif //LOOM_EDITOR_EDITOR_WINDOW_H
