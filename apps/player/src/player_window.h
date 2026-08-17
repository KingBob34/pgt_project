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
class QPushButton;
class QTextEdit;
class QTreeWidget;
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

    // The panel is not in a layout, so it is resized with what it covers.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildMenus();
    void buildLayout();

    // The row of system buttons and the panel one of them opens. Both live in
    // the game surface, because they are things the player uses.
    QWidget* buildSystemBar();
    void     buildStatus(QWidget* surface);

    // Raises the panel over the darkened story, or puts it away again.
    void showStatus(bool on);

    // Redraws the panel from the running story. Called wherever the story has
    // just stopped to wait, which is the only time the player can look.
    void refreshStatus();

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
    QWidget*     statusOverlay = nullptr;
    QPushButton* statusButton = nullptr;
    QTreeWidget* status = nullptr;
};

#endif //LOOM_PLAYER_PLAYER_WINDOW_H
