#ifndef LOOM_PLAYER_PLAYER_WINDOW_H
#define LOOM_PLAYER_PLAYER_WINDOW_H
#include <memory>
#include <string>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "loom/graph/catalog.h"
#include "loom/graph/graph.h"
#include "loom/runtime/interpreter.h"

class QListWidget;
class QTextEdit;
class QVBoxLayout;

// The game window. Implements Host, the engine's one route to a front end.
class PlayerWindow : public QMainWindow, public loom::Host
{
    Q_OBJECT

public:
    PlayerWindow();

    void openStory(const QString& path);

    void showText(const std::string& text, const loom::TextStyle& style) override;
    void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle& style) override;
    void command(const std::string& name, const loom::Value& args) override;

private:
    void buildMenus();
    void buildLayout();

    void chooseOption(int index);
    void clearChoices();
    void log(const QString& text, bool fault = false);

    void openStoryDialog();
    void saveGame();
    void loadGame();

    loom::NodeCatalog                  catalog;
    loom::Project                      project;
    std::unique_ptr<loom::Interpreter> interpreter;
    QString                            storyPath;

    QTextEdit*   passage = nullptr;
    QWidget*     choices = nullptr;
    QVBoxLayout* choiceRow = nullptr;
    QListWidget* console = nullptr;
};

#endif //LOOM_PLAYER_PLAYER_WINDOW_H
