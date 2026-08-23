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
#include <QPointer>
#include <QString>

#include <QtNodes/NodeDelegateModelRegistry>

#include "canvas_faults.h"

#include "loom/graph/catalog.h"
#include "loom/graph/diagnostics.h"
#include "loom/graph/graph.h"

class DetailsPanel;
class GraphDocument;
class GraphModel;
class GraphScene;
class GraphView;
class PlaytestPanel;
class ValueTree;
class QDockWidget;
class QListWidget;
class QListWidgetItem;
class QAction;
class QMenu;
class InlineEdit;
class QTabBar;
class QTabWidget;

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow();
    ~EditorWindow() override;

    void openStory(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

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

    // The project as it stands on the canvas, ready to be played or written.
    void gatherProject();

    void playStory();
    void playStoryHere();

    // Writes a folder the author can hand to a player: the game, its story and
    // the libraries both need.
    void exportGame();

    void clearConsole();

    void setStoryPath(const QString& path);

    QListWidgetItem* log(const QString& text, bool fault = false);

    // A console line that remembers the node it is about.
    void logAt(const QString& text, const std::string& graph, loom::NodeId node, bool fault = false);

    QString nodeLabel(const std::string& graph, loom::NodeId node) const;
    void    revealNode(QListWidgetItem* line);

    // Brings one node into view, switching scenes if it is in another one.
    bool focusNode(const std::string& graph, loom::NodeId node);

    void report(const loom::Diagnostics& diagnostics);

    void syncDetails();

    // The Details tab exists only while a node is selected: an empty panel
    // saying it is empty is a tab the author has to learn to ignore.
    void showDetails(bool on);

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
    void renameScene(int index);

    // Display order only. Which scene the story starts in is 'entry', not the
    // first tab, so the author may keep them in whatever order reads best.
    void reorderScenes(int from, int to);

    // Applies a name typed on a tab. Takes the old name rather than an index
    // because the tabs may have been dragged about while the box was open.
    void takeSceneName(const std::string& was, const std::string& typed);

    // Puts away the rename box, if one is up.
    void closeSceneBox();

    std::string uniqueSceneName(const std::string& wanted) const;
    void reportSceneReferences(const std::string& name);

    // Every node builds its menus from the story's own names, so all of them
    // are rebuilt when one arrives or leaves.
    void refreshNodeEditors();

    // The whole story as it would be written out this moment.
    std::string snapshot();

    // Asks only when there is something to lose, and answers false when the
    // author chose to stay.
    bool mayDiscard();

    loom::NodeCatalog catalog;

    // Read by the painters every time the canvas redraws, so it outlives the
    // scene it colours.
    CanvasFaults faults;

    // Declared first: the registry hands a reference to each of these to every
    // node, so a pin that picks a name out of a list is always offering the
    // list as it stands.
    std::map<std::string, loom::VariableSpec> variableSpecs;
    std::vector<std::string>                  sceneNames;

    // What the nodes are currently offering, each name with its type, to tell a
    // stale menu from a fresh one.
    std::vector<std::pair<std::string, std::string>> offeredMenu;

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry;
    std::unique_ptr<GraphModel>                         model;
    std::unique_ptr<GraphDocument>                      document;

    GraphScene* scene = nullptr;
    GraphView*  view  = nullptr;

    QListWidget*   console = nullptr;
    QTabBar*       scenes = nullptr;

    // The box renaming a scene, while one is up. It is drawn over one tab and
    // means nothing once the tabs have moved under it.
    QPointer<InlineEdit> sceneBox;

    QTabWidget*    panel = nullptr;
    DetailsPanel*  details = nullptr;
    ValueTree*     values = nullptr;
    PlaytestPanel* playtest = nullptr;
    QDockWidget*   playtestDock = nullptr;
    QMenu*         panelMenu = nullptr;
    QAction*       saveAction = nullptr;
    QAction*       playAction = nullptr;
    QAction*       playHereAction = nullptr;

    // True while the scene list is being refilled, so its signals can be ignored.
    bool rebuilding = false;

    loom::Project project;
    std::size_t   editing = 0;

    QString storyPath;

    // The story as it stood when it was last written or opened. What is in
    // front of the author is compared against this, which catches every way a
    // change can be made rather than the ways someone thought to watch for.
    std::string saved;
};

#endif //LOOM_EDITOR_EDITOR_WINDOW_H
