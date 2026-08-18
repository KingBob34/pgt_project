#ifndef LOOM_EDITOR_PLAYTEST_PANEL_H
#define LOOM_EDITOR_PLAYTEST_PANEL_H
#include <memory>
#include <string>
#include <vector>

#include <QWidget>

#include "loom/graph/catalog.h"
#include "loom/graph/graph.h"
#include "loom/runtime/interpreter.h"

class QTextEdit;
class QVBoxLayout;

// The story running inside the editor. Implements Host, the same one the
// game window does.
class PlaytestPanel : public QWidget, public loom::Host
{
    Q_OBJECT

public:
    explicit PlaytestPanel(const loom::NodeCatalog& catalog, QWidget* parent = nullptr);
    ~PlaytestPanel() override;

    // Runs the project from its entry point, or from one node of one scene.
    void play(const loom::Project& project);
    void playFrom(const loom::Project& project, const std::string& graph, loom::NodeId node);

    void stop();

    void showText(const std::string& text, const loom::TextStyle& style) override;
    void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle& style) override;
    void command(const std::string& name, const loom::Value& args) override;

Q_SIGNALS:
    // An engine fault, named to the node that was running when it happened so
    // the console line can lead back to it.
    void faulted(const QString& detail, const QString& graph, loom::NodeId node);

private:
    // Takes its own copy of the project and points a fresh interpreter at it.
    // The copy matters: the author keeps editing while the story runs.
    void begin(const loom::Project& source);

    void chooseOption(int index);
    void clearChoices();

    const loom::NodeCatalog& catalog;

    loom::Project                      project;
    std::unique_ptr<loom::Interpreter> interpreter;

    QTextEdit*   passage = nullptr;
    QVBoxLayout* choiceRow = nullptr;
    QWidget*     choices = nullptr;
};

#endif //LOOM_EDITOR_PLAYTEST_PANEL_H
