#include "player_window.h"

#include <QAction>
#include <QColor>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "loom/nodes/builtin.h"
#include "loom/runtime/save.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }

    // Colour components are written as 0.0 to 1.0, but JSON drops a zero
    // fraction, so a hand written 1 has to read the same as 1.0.
    int channel(const loom::Value& source, const std::string& key)
    {
        const loom::Value* component = loom::objectGet(source, key);
        if (component == nullptr) return 0;

        const double scaled = loom::isInt(*component)
                            ? static_cast<double>(loom::asInt(*component))
                            : loom::asFloat(*component);

        return static_cast<int>(scaled * 255.0);
    }

    QColor toColor(const loom::Value& value)
    {
        if (!loom::isObject(value)) return QColor(Qt::black);

        return QColor(channel(value, "r"), channel(value, "g"),
                      channel(value, "b"), channel(value, "a"));
    }

    bool readFile(const QString& path, std::string& out, QString& error)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            error = file.errorString();
            return false;
        }

        const QByteArray bytes = file.readAll();
        out.assign(bytes.constData(), static_cast<std::size_t>(bytes.size()));

        return true;
    }

    bool writeFile(const QString& path, const std::string& text, QString& error)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            error = file.errorString();
            return false;
        }

        file.write(text.data(), static_cast<qint64>(text.size()));

        return true;
    }
}

PlayerWindow::PlayerWindow()
{
    loom::registerBuiltinNodes(catalog);

    setWindowTitle("Loom Player");
    resize(900, 700);

    buildLayout();
    buildMenus();
}

void PlayerWindow::buildLayout()
{
    passage = new QTextEdit;
    passage->setReadOnly(true);
    passage->setFrameStyle(QFrame::NoFrame);
    passage->setStyleSheet("QTextEdit { background: white; padding: 24px; }");

    choiceRow = new QHBoxLayout;
    choiceRow->setContentsMargins(24, 12, 24, 24);
    choiceRow->setSpacing(12);

    choices = new QWidget;
    choices->setLayout(choiceRow);
    choices->setStyleSheet("QWidget { background: white; }");

    QWidget* centre = new QWidget;
    QVBoxLayout* column = new QVBoxLayout(centre);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(passage, 1);
    column->addWidget(choices, 0);

    setCentralWidget(centre);

    console = new QListWidget;

    QDockWidget* dock = new QDockWidget("Console", this);
    dock->setWidget(console);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void PlayerWindow::buildMenus()
{
    QMenu* file = menuBar()->addMenu("&File");

    QAction* open = file->addAction("&Open Story...", this, &PlayerWindow::openStoryDialog);
    open->setShortcut(QKeySequence::Open);

    file->addSeparator();
    file->addAction("&Save Game...", this, &PlayerWindow::saveGame);
    file->addAction("&Load Game...", this, &PlayerWindow::loadGame);

    file->addSeparator();
    QAction* quit = file->addAction("&Quit", this, &QWidget::close);
    quit->setShortcut(QKeySequence::Quit);
}

void PlayerWindow::log(const QString& text, bool fault)
{
    QListWidgetItem* line = new QListWidgetItem(text, console);
    if (fault) line->setForeground(Qt::red);

    console->scrollToBottom();
}

void PlayerWindow::openStoryDialog()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Story", QString(), "Loom story (*.loom *.json);;All files (*)");

    if (!path.isEmpty()) openStory(path);
}

void PlayerWindow::openStory(const QString& path)
{
    interpreter.reset();
    clearChoices();
    passage->clear();
    console->clear();

    std::string text;
    QString failure;
    if (!readFile(path, text, failure))
    {
        log("Cannot read " + path + ": " + failure, true);
        return;
    }

    loom::Value document;
    std::string error;
    if (!loom::parseJson(text, document, error))
    {
        log(toQt(error), true);
        return;
    }

    project = loom::Project();

    loom::Diagnostics diagnostics;
    const bool read = loom::readProject(document, catalog, project, diagnostics);

    for (const loom::Diagnostic& entry : diagnostics.all())
    {
        QString line = toQt(entry.message);
        if (entry.node != 0) line += QString(" (node %1)").arg(entry.node);
        if (!entry.pin.empty()) line += QString(" [%1]").arg(toQt(entry.pin));

        log(line, entry.severity == loom::Severity::Error);
    }

    // Warnings are what an unfinished story looks like, so only errors stop it.
    if (!read || diagnostics.hasErrors())
    {
        log("The story has faults and will not be played.", true);
        return;
    }

    storyPath = path;
    setWindowTitle("Loom Player - " + toQt(project.meta.title));

    interpreter = std::make_unique<loom::Interpreter>(project, catalog, *this);
    interpreter->start();
}

void PlayerWindow::showText(const std::string& text, const loom::TextStyle& style)
{
    passage->append(QString("<p style=\"font-size:%1pt; color:%2;\">%3</p>")
                        .arg(style.fontSize)
                        .arg(toColor(style.color).name())
                        .arg(toQt(text).toHtmlEscaped()));
}

void PlayerWindow::askChoice(const std::vector<loom::Option>& options, const loom::TextStyle& style)
{
    clearChoices();

    for (std::size_t index = 0; index < options.size(); ++index)
    {
        QPushButton* button = new QPushButton(toQt(options[index].text));
        button->setStyleSheet(QString("QPushButton { font-size:%1pt; padding: 10px 18px; }")
                                  .arg(style.fontSize));

        const int picked = static_cast<int>(index);
        connect(button, &QPushButton::clicked, this, [this, picked] { chooseOption(picked); });

        choiceRow->addWidget(button);
    }
}

void PlayerWindow::command(const std::string& name, const loom::Value& args)
{
    if (name == "error")
    {
        const loom::Value* node = loom::objectGet(args, "node");
        const loom::Value* detail = loom::objectGet(args, "detail");

        log(QString("%1: %2")
                .arg(node == nullptr ? QString("engine") : toQt(loom::asString(*node)))
                .arg(detail == nullptr ? QString() : toQt(loom::asString(*detail))),
            true);
        return;
    }

    if (name == "print")
    {
        const loom::Value* text = loom::objectGet(args, "text");
        log(text == nullptr ? QString() : toQt(loom::asString(*text)));
        return;
    }

    log(toQt(name) + " " + toQt(loom::toText(args)));
}

void PlayerWindow::clearChoices()
{
    while (QLayoutItem* item = choiceRow->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
}

void PlayerWindow::chooseOption(int index)
{
    if (interpreter == nullptr) return;

    clearChoices();

    // The reader shows one passage at a time: what came before is finished with.
    passage->clear();

    interpreter->choose(index);

    if (interpreter->finished()) log("The story has ended.");
}

void PlayerWindow::saveGame()
{
    if (interpreter == nullptr) return;

    const QString path = QFileDialog::getSaveFileName(
        this, "Save Game", QString(), "Loom save (*.loomsave)");

    if (path.isEmpty()) return;

    QString failure;
    if (!writeFile(path, loom::writeJson(loom::writeSave(interpreter->save())), failure))
    {
        log("Cannot write " + path + ": " + failure, true);
        return;
    }

    log("Saved to " + path);
}

void PlayerWindow::loadGame()
{
    if (interpreter == nullptr)
    {
        log("Open a story before loading a save.", true);
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, "Load Game", QString(), "Loom save (*.loomsave)");

    if (path.isEmpty()) return;

    std::string text;
    QString failure;
    if (!readFile(path, text, failure))
    {
        log("Cannot read " + path + ": " + failure, true);
        return;
    }

    loom::Value document;
    std::string error;
    if (!loom::parseJson(text, document, error))
    {
        log(toQt(error), true);
        return;
    }

    loom::SaveState state;
    if (!loom::readSave(document, state, error))
    {
        log(toQt(error), true);
        return;
    }

    clearChoices();
    passage->clear();

    interpreter->restore(state);

    // Restoring lands on the node that was waiting, so the story has to be
    // nudged for the buttons of that moment to be put back on screen.
    interpreter->replay();
}
