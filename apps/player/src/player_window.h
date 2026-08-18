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

class QPushButton;
class QTextEdit;
class QTreeWidget;
class QVBoxLayout;

// The game window. Implements Host, the engine's one route to a front end.
// Nothing in here belongs to the editor: what the author sees when they
// export a game is exactly this.
class PlayerWindow : public QMainWindow, public loom::Host
{
    Q_OBJECT

public:
    PlayerWindow();

    void openStory(const QString& path);

    // The story this game was built around: the one named after the game, or
    // the only one sitting beside it.
    void openStoryBesideMe();

    void showText(const std::string& text, const loom::TextStyle& style) override;
    void askChoice(const std::vector<loom::Option>& options, const loom::TextStyle& style) override;
    void command(const std::string& name, const loom::Value& args) override;

    // The panels are not in a layout, so they are resized with what they cover.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildLayout();

    // The row of system buttons and the two panels they open. All of it lives
    // in the game surface, because it is what the player uses.
    QWidget* buildSystemBar();
    void     buildStatus(QWidget* surface);
    void     buildSettings(QWidget* surface);

    // Raises one panel over the darkened story, or puts it away again.
    void showOverlay(QWidget* overlay, bool on);

    // Redraws the status panel from the running story.
    void refreshStatus();

    void chooseOption(int index);
    void clearChoices();

    // Says something the player needs to know. A game has no console: what
    // goes wrong at runtime is the author's to find in the editor.
    void report(const QString& text);

    void saveGame();
    void loadGame();

    loom::NodeCatalog                  catalog;
    loom::Project                      project;
    std::unique_ptr<loom::Interpreter> interpreter;
    QString                            storyPath;

    QTextEdit*   passage = nullptr;
    QWidget*     choices = nullptr;
    QVBoxLayout* choiceRow = nullptr;
    QWidget*     systemBar = nullptr;
    QWidget*     statusOverlay = nullptr;
    QWidget*     settingsOverlay = nullptr;
    QTreeWidget* status = nullptr;
};

#endif //LOOM_PLAYER_PLAYER_WINDOW_H
