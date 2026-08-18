#include "player_window.h"

#include <QAction>
#include <QColor>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QAbstractItemView>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "loom/nodes/builtin.h"
#include "loom/runtime/save.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    // A line of prose is read by its length, not by the window's. Past about
    // seventy-five characters the eye loses its place coming back, so the story
    // stops widening there and the window keeps the rest as margin.
    constexpr int kReaderWidth = 760;

    constexpr int kStatusIndent = 12;
    constexpr int kStatusWidth = 420;
    constexpr int kStatusHeight = 480;

    // The tree draws itself from the desktop palette unless it is told not to,
    // and the story around it is white whatever the desktop theme is.
    const char* const kStatusTreeStyle =
        "QTreeWidget {"
        "  background: white;"
        "  color: #1a1a1a;"
        "  border: 1px solid #e0e0e0;"
        "}"
        "QTreeWidget::item { padding: 3px 0; }"
        "QTreeWidget::item:selected { background: #dce8f4; color: #1a1a1a; }"
        "QHeaderView::section {"
        "  background: #f0f0f0;"
        "  color: #4a4a4a;"
        "  border: none;"
        "  border-bottom: 1px solid #e0e0e0;"
        "  padding: 5px;"
        "}";

    const char* const kCloseButtonStyle =
        "QPushButton {"
        "  color: #6a6a6a;"
        "  background: transparent;"
        "  border: none;"
        "  font-size: 15pt;"
        "  padding: 0 6px;"
        "}"
        "QPushButton:hover { color: #1a1a1a; }";

    // Quieter than a choice: it is a way to look at the game, not to play it.
    const char* const kSystemButtonStyle =
        "QPushButton {"
        "  color: #4a4a4a;"
        "  background: #f2f2f2;"
        "  border: 1px solid #c8c8c8;"
        "  border-radius: 4px;"
        "  padding: 4px 14px;"
        "}"
        "QPushButton:hover { background: #e4e4e4; border-color: #8c8c8c; }";

    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }

    QString countOf(int count, const QString& noun)
    {
        return QString::number(count) + " " + noun + (count == 1 ? "" : "s");
    }

    // A leaf states what it holds; a container states how much, and spells its
    // rows out underneath.
    void fillValue(QTreeWidgetItem* row, const loom::Value& value)
    {
        if (loom::isList(value))
        {
            for (std::size_t index = 0; index < loom::listSize(value); ++index)
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(row);
                item->setText(0, QString::number(index));

                fillValue(item, *loom::listAt(value, index));
            }

            row->setText(1, countOf(row->childCount(), "item"));
            return;
        }

        if (loom::isObject(value))
        {
            for (const std::string& key : loom::objectKeys(value))
            {
                QTreeWidgetItem* field = new QTreeWidgetItem(row);
                field->setText(0, toQt(key));

                fillValue(field, *loom::objectGet(value, key));
            }

            row->setText(1, countOf(row->childCount(), "field"));
            return;
        }

        row->setText(1, toQt(loom::toText(value)));
    }

    // Colour components run from 0.0 to 1.0, and JSON drops a zero fraction.
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
    setMinimumSize(640, 480);
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

    // Stacked, not side by side: an option is often a whole sentence, and a
    // row of them squeezes every button down to nothing.
    choiceRow = new QVBoxLayout;
    choiceRow->setContentsMargins(24, 12, 24, 24);
    choiceRow->setSpacing(8);

    choices = new QWidget;
    choices->setLayout(choiceRow);

    // Named, because a bare QWidget selector would repaint every button in here
    // as well and leave them white on white.
    choices->setObjectName("choiceBar");
    choices->setAttribute(Qt::WA_StyledBackground, true);
    choices->setStyleSheet("#choiceBar { background: white; }");

    QWidget* reader = new QWidget;
    reader->setMaximumWidth(kReaderWidth);

    QVBoxLayout* column = new QVBoxLayout(reader);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(buildSystemBar(), 0);
    column->addWidget(passage, 1);
    column->addWidget(choices, 0);

    QWidget* centre = new QWidget;

    // The story takes what it can up to its own limit; the stretches take back
    // whatever is left over, which is what centres it on a wide screen.
    QHBoxLayout* across = new QHBoxLayout(centre);
    across->setContentsMargins(0, 0, 0, 0);
    across->setSpacing(0);
    across->addStretch();
    across->addWidget(reader, 1);
    across->addStretch();

    centre->setObjectName("gameSurface");
    centre->setAttribute(Qt::WA_StyledBackground, true);
    centre->setStyleSheet("#gameSurface { background: white; }");

    buildStatus(centre);

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

QWidget* PlayerWindow::buildSystemBar()
{
    statusButton = new QPushButton("Status");
    statusButton->setStyleSheet(kSystemButtonStyle);

    // Kept out of the way while the panel is up, without the story shifting
    // into the space it leaves.
    QSizePolicy policy = statusButton->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    statusButton->setSizePolicy(policy);

    connect(statusButton, &QPushButton::clicked, this, [this] { showStatus(true); });

    QWidget* bar = new QWidget;

    QHBoxLayout* row = new QHBoxLayout(bar);
    row->setContentsMargins(24, 12, 24, 0);
    row->addStretch();
    row->addWidget(statusButton);

    // Named, or a bare QWidget selector would repaint every button in here.
    bar->setObjectName("systemBar");
    bar->setAttribute(Qt::WA_StyledBackground, true);
    bar->setStyleSheet("#systemBar { background: white; }");

    return bar;
}

void PlayerWindow::buildStatus(QWidget* surface)
{
    status = new QTreeWidget;
    status->setColumnCount(2);
    status->setHeaderLabels({ "Name", "Value" });
    status->setEditTriggers(QAbstractItemView::NoEditTriggers);
    status->setIndentation(kStatusIndent);
    status->setStyleSheet(kStatusTreeStyle);

    status->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    status->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QLabel* title = new QLabel("Status");
    title->setStyleSheet("color: #1a1a1a; font-size: 13pt;");

    QPushButton* close = new QPushButton(QString::fromUtf8("\xC3\x97"));
    close->setStyleSheet(kCloseButtonStyle);
    close->setCursor(Qt::PointingHandCursor);

    connect(close, &QPushButton::clicked, this, [this] { showStatus(false); });

    QHBoxLayout* heading = new QHBoxLayout;
    heading->setContentsMargins(0, 0, 0, 0);
    heading->addWidget(title);
    heading->addStretch();
    heading->addWidget(close);

    QWidget* card = new QWidget;
    card->setFixedSize(kStatusWidth, kStatusHeight);

    QVBoxLayout* inside = new QVBoxLayout(card);
    inside->setContentsMargins(16, 12, 16, 16);
    inside->setSpacing(10);
    inside->addLayout(heading);
    inside->addWidget(status, 1);

    card->setObjectName("statusCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("#statusCard { background: white; border-radius: 6px; }");

    // A child of the game surface, not a window: the story stays behind it,
    // dimmed, so the player can see they are still in the same place.
    statusOverlay = new QWidget(surface);
    statusOverlay->setObjectName("statusScrim");
    statusOverlay->setAttribute(Qt::WA_StyledBackground, true);
    statusOverlay->setStyleSheet("#statusScrim { background: rgba(0, 0, 0, 110); }");

    QVBoxLayout* middle = new QVBoxLayout(statusOverlay);
    middle->addWidget(card, 0, Qt::AlignCenter);

    statusOverlay->hide();

    surface->installEventFilter(this);
}

void PlayerWindow::showStatus(bool on)
{
    statusButton->setVisible(!on);

    if (on)
    {
        refreshStatus();

        statusOverlay->setGeometry(statusOverlay->parentWidget()->rect());
        statusOverlay->raise();
    }

    statusOverlay->setVisible(on);
}

bool PlayerWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Resize && statusOverlay != nullptr &&
        watched == statusOverlay->parentWidget())
    {
        statusOverlay->setGeometry(statusOverlay->parentWidget()->rect());
    }

    return QMainWindow::eventFilter(watched, event);
}

void PlayerWindow::refreshStatus()
{
    status->clear();

    if (interpreter == nullptr) return;

    for (const auto& entry : interpreter->state())
    {
        QTreeWidgetItem* row = new QTreeWidgetItem(status);
        row->setText(0, toQt(entry.first));

        fillValue(row, entry.second);
    }

    status->expandAll();
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

    refreshStatus();
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

        // The passage is white whatever the desktop theme is, so the buttons
        // state their own colours instead of inheriting a dark palette.
        button->setStyleSheet(QString("QPushButton {"
                                      "  font-size: %1pt;"
                                      "  color: #1a1a1a;"
                                      "  background: #f2f2f2;"
                                      "  border: 1px solid #b4b4b4;"
                                      "  border-radius: 4px;"
                                      "  padding: 10px 18px;"
                                      "  text-align: left;"
                                      "}"
                                      "QPushButton:hover { background: #e4e4e4; border-color: #7a7a7a; }"
                                      "QPushButton:pressed { background: #d2d2d2; }"
                                      "QPushButton:disabled {"
                                      "  color: #a0a0a0;"
                                      "  background: #ececec;"
                                      "  border-color: #d8d8d8;"
                                      "}")
                                  .arg(style.fontSize));

        // Locked, not gone: the route stays on screen until the story opens it.
        button->setEnabled(options[index].enabled);

        const int picked = static_cast<int>(index);
        connect(button, &QPushButton::clicked, this, [this, picked] { chooseOption(picked); });

        choiceRow->addWidget(button);

        // A widget born without a parent starts hidden, and being put in a
        // layout does not undo that.
        button->show();
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
        // Deferred: this runs from the clicked button's own handler.
        if (QWidget* widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }

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

    refreshStatus();

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

    // Re-runs the node that was waiting, which puts its buttons back on screen.
    interpreter->replay();

    refreshStatus();
}
