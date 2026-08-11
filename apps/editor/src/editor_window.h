#ifndef LOOM_EDITOR_EDITOR_WINDOW_H
#define LOOM_EDITOR_EDITOR_WINDOW_H
#include <cstddef>
#include <memory>

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

    void newStory();
    void chooseStory();
    bool saveStory();
    bool saveStoryAs();
    bool writeStory(const QString& path);

    void setStoryPath(const QString& path);
    void showDiagnostics(const loom::Diagnostics& diagnostics, const QString& heading);

    loom::NodeCatalog catalog;

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry;
    std::unique_ptr<GraphModel>                         model;
    std::unique_ptr<GraphDocument>                      document;

    GraphScene* scene = nullptr;
    GraphView*  view  = nullptr;

    loom::Project project;
    std::size_t   editing = 0;

    QString storyPath;
};

#endif //LOOM_EDITOR_EDITOR_WINDOW_H
