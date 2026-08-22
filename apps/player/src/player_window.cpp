#include "player_window.h"

#include "option_button.h"

#include <functional>

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "loom/nodes/builtin.h"
#include "loom/qt/convert.h"
#include "loom/qt/passage.h"
#include "loom/runtime/save.h"
#include "loom/serialization/graph_io.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

using loom::qt::toQt;

namespace
{
    constexpr int kStatusIndent = 12;
    constexpr int kPanelWidth = 420;
    constexpr int kStatusHeight = 480;
    constexpr int kSettingsHeight = 240;
    constexpr int kEndingHeight = 300;

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

    // Options are unstyled by the author, so the shell settles their size.
    constexpr int kOptionSize = 16;

    // The passage is white whatever the desktop theme is, so the buttons state
    // their own colours instead of inheriting a dark palette.
    QString optionStyle(int fontSize)
    {
        return QString("QPushButton {"
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
                       "}").arg(fontSize);
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

    // A panel that covers the game and dims it: one white card in the middle,
    // with a title and a way out. Hidden until something raises it.
    QWidget* makeOverlay(QWidget* surface, const QString& title, QWidget* body,
                         int height, const std::function<void()>& closed)
    {
        QLabel* heading = new QLabel(title);
        heading->setStyleSheet("color: #1a1a1a; font-size: 13pt;");

        QPushButton* close = new QPushButton(QString::fromUtf8("\xC3\x97"));
        close->setStyleSheet(kCloseButtonStyle);
        close->setCursor(Qt::PointingHandCursor);

        QObject::connect(close, &QPushButton::clicked, surface, closed);

        QHBoxLayout* top = new QHBoxLayout;
        top->setContentsMargins(0, 0, 0, 0);
        top->addWidget(heading);
        top->addStretch();
        top->addWidget(close);

        QWidget* card = new QWidget;
        card->setFixedSize(kPanelWidth, height);

        QVBoxLayout* inside = new QVBoxLayout(card);
        inside->setContentsMargins(16, 12, 16, 16);
        inside->setSpacing(10);
        inside->addLayout(top);
        inside->addWidget(body, 1);

        card->setObjectName("panelCard");
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setStyleSheet("#panelCard { background: white; border-radius: 6px; }");

        // A child of the game surface, not a window: the story stays behind it,
        // dimmed, so the player can see they are still in the same place.
        QWidget* overlay = new QWidget(surface);
        overlay->setObjectName("panelScrim");
        overlay->setAttribute(Qt::WA_StyledBackground, true);
        overlay->setStyleSheet("#panelScrim { background: rgba(0, 0, 0, 110); }");

        QVBoxLayout* middle = new QVBoxLayout(overlay);
        middle->addWidget(card, 0, Qt::AlignCenter);

        overlay->hide();

        return overlay;
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

    setWindowTitle("Loom");
    setMinimumSize(640, 480);
    resize(900, 700);

    buildLayout();
}

void PlayerWindow::buildLayout()
{
    passage = new QTextEdit;
    passage->setReadOnly(true);
    passage->setFrameStyle(QFrame::NoFrame);
    passage->setStyleSheet("QTextEdit { background: white; padding: 40px 72px; }");

    // Stacked, not side by side: an option is often a whole sentence, and a
    // row of them squeezes every button down to nothing.
    choiceRow = new QVBoxLayout;
    choiceRow->setContentsMargins(72, 12, 72, 40);
    choiceRow->setSpacing(8);

    choices = new QWidget;
    choices->setLayout(choiceRow);

    // Named, because a bare QWidget selector would repaint every button in here
    // as well and leave them white on white.
    choices->setObjectName("choiceBar");
    choices->setAttribute(Qt::WA_StyledBackground, true);
    choices->setStyleSheet("#choiceBar { background: white; }");

    QWidget* centre = new QWidget;

    QVBoxLayout* column = new QVBoxLayout(centre);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(buildSystemBar(), 0);
    column->addWidget(passage, 1);
    column->addWidget(choices, 0);

    centre->setObjectName("gameSurface");
    centre->setAttribute(Qt::WA_StyledBackground, true);
    centre->setStyleSheet("#gameSurface { background: white; }");

    buildStatus(centre);
    buildSettings(centre);
    buildEnding(centre);

    centre->installEventFilter(this);

    setCentralWidget(centre);
}

QWidget* PlayerWindow::buildSystemBar()
{
    QPushButton* toStatus = new QPushButton("Status");
    toStatus->setStyleSheet(kSystemButtonStyle);

    connect(toStatus, &QPushButton::clicked, this,
            [this] { showOverlay(statusOverlay, true); });

    QPushButton* toSettings = new QPushButton("Settings");
    toSettings->setStyleSheet(kSystemButtonStyle);

    connect(toSettings, &QPushButton::clicked, this,
            [this] { showOverlay(settingsOverlay, true); });

    systemBar = new QWidget;

    // Kept out of the way while a panel is up, without the story shifting into
    // the space it leaves.
    QSizePolicy policy = systemBar->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    systemBar->setSizePolicy(policy);

    QHBoxLayout* row = new QHBoxLayout(systemBar);
    row->setContentsMargins(24, 12, 24, 0);
    row->setSpacing(8);
    row->addStretch();
    row->addWidget(toStatus);
    row->addWidget(toSettings);

    // Named, or a bare QWidget selector would repaint every button in here.
    systemBar->setObjectName("systemBar");
    systemBar->setAttribute(Qt::WA_StyledBackground, true);
    systemBar->setStyleSheet("#systemBar { background: white; }");

    return systemBar;
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

    statusOverlay = makeOverlay(surface, "Status", status, kStatusHeight,
                                [this] { showOverlay(statusOverlay, false); });
}

void PlayerWindow::buildSettings(QWidget* surface)
{
    QWidget* body = new QWidget;

    QVBoxLayout* column = new QVBoxLayout(body);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(8);

    const auto entry = [&](const QString& text, void (PlayerWindow::*done)())
    {
        QPushButton* button = new QPushButton(text);
        button->setStyleSheet(kSystemButtonStyle);

        connect(button, &QPushButton::clicked, this, done);

        column->addWidget(button);
    };

    entry("Save Game", &PlayerWindow::saveGame);
    entry("Load Game", &PlayerWindow::loadGame);

    QPushButton* quit = new QPushButton("Quit");
    quit->setStyleSheet(kSystemButtonStyle);

    connect(quit, &QPushButton::clicked, this, &QWidget::close);

    column->addWidget(quit);
    column->addStretch();

    settingsOverlay = makeOverlay(surface, "Settings", body, kSettingsHeight,
                                  [this] { showOverlay(settingsOverlay, false); });
}

void PlayerWindow::buildEnding(QWidget* surface)
{
    endingText = new QLabel;
    endingText->setWordWrap(true);
    endingText->setAlignment(Qt::AlignCenter);
    endingText->setStyleSheet("color: #1a1a1a; font-size: 14pt;");

    QPushButton* again = new QPushButton("Play Again");
    again->setStyleSheet(kSystemButtonStyle);

    connect(again, &QPushButton::clicked, this, &PlayerWindow::restart);

    QPushButton* quit = new QPushButton("Quit");
    quit->setStyleSheet(kSystemButtonStyle);

    connect(quit, &QPushButton::clicked, this, &QWidget::close);

    QHBoxLayout* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    row->addStretch();
    row->addWidget(again);
    row->addWidget(quit);
    row->addStretch();

    QWidget* body = new QWidget;

    QVBoxLayout* column = new QVBoxLayout(body);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(16);
    column->addStretch();
    column->addWidget(endingText);
    column->addStretch();
    column->addLayout(row);

    endingOverlay = makeOverlay(surface, "The End", body, kEndingHeight,
                                [this] { showOverlay(endingOverlay, false); });
}

void PlayerWindow::showEnding(const QString& text)
{
    endingText->setText(text);

    showOverlay(endingOverlay, true);
}

void PlayerWindow::restart()
{
    if (interpreter == nullptr) return;

    showOverlay(endingOverlay, false);

    clearChoices();
    passage->clear();

    interpreter->start();
}

void PlayerWindow::showOverlay(QWidget* overlay, bool on)
{
    systemBar->setVisible(!on);

    if (on)
    {
        if (overlay == statusOverlay) refreshStatus();

        overlay->setGeometry(overlay->parentWidget()->rect());
        overlay->raise();
    }

    overlay->setVisible(on);
}

bool PlayerWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Resize)
    {
        for (QWidget* overlay : { statusOverlay, settingsOverlay, endingOverlay })
        {
            if (overlay != nullptr && watched == overlay->parentWidget())
            {
                overlay->setGeometry(overlay->parentWidget()->rect());
            }
        }
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

void PlayerWindow::report(const QString& text)
{
    QMessageBox::warning(this, windowTitle(), text);
}

void PlayerWindow::openStoryBesideMe()
{
    const QFileInfo self(QCoreApplication::applicationFilePath());
    const QDir here = self.absoluteDir();

    // Named after the game first, because a folder may hold more than one.
    const QString named = here.filePath(self.completeBaseName() + ".loom");

    if (QFileInfo::exists(named))
    {
        openStory(named);
        return;
    }

    const QStringList found = here.entryList({ "*.loom" }, QDir::Files, QDir::Name);

    if (found.size() == 1)
    {
        openStory(here.filePath(found.front()));
        return;
    }

    report("No story was found next to this game.");

    const QString chosen = QFileDialog::getOpenFileName(
        this, "Open Story", here.absolutePath(), "Loom story (*.loom);;All files (*)");

    if (!chosen.isEmpty()) openStory(chosen);
}

void PlayerWindow::openStory(const QString& path)
{
    interpreter.reset();
    clearChoices();
    passage->clear();

    std::string text;
    QString     failure;
    if (!readFile(path, text, failure))
    {
        report("Cannot read " + path + ":\n" + failure);
        return;
    }

    loom::Value document;
    std::string error;
    if (!loom::parseJson(text, document, error))
    {
        report(toQt(error));
        return;
    }

    project = loom::Project();

    loom::Diagnostics diagnostics;
    const bool read = loom::readProject(document, catalog, project, diagnostics);

    // Warnings are what an unfinished story looks like, so only errors stop it,
    // and only then does the player hear about any of this.
    if (!read || diagnostics.hasErrors())
    {
        QStringList faults;

        for (const loom::Diagnostic& entry : diagnostics.all())
        {
            if (entry.severity != loom::Severity::Error) continue;

            QString line = toQt(entry.message);
            if (entry.node != 0) line += QString(" (node %1)").arg(entry.node);

            faults << line;
        }

        report("This story cannot be played.\n\n" + faults.join("\n"));
        return;
    }

    storyPath = path;
    setWindowTitle(toQt(project.meta.title));

    interpreter = std::make_unique<loom::Interpreter>(project, catalog, *this);
    interpreter->start();
}

void PlayerWindow::showText(const std::vector<loom::TextRun>& runs)
{
    passage->append(loom::qt::passageHtml(runs));
}

void PlayerWindow::askChoice(const std::vector<loom::Option>& options)
{
    clearChoices();

    for (std::size_t index = 0; index < options.size(); ++index)
    {
        OptionButton* button = new OptionButton(toQt(options[index].text), kOptionSize);
        button->setStyleSheet(optionStyle(kOptionSize));

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
    // An ending is the one thing the engine says that is meant for the player.
    // Faults are not: the author meets those in the editor's console, where
    // they lead back to the node that raised them.
    if (name != "ending") return;

    const loom::Value* text = loom::objectGet(args, "text");

    showEnding(text == nullptr ? QString() : toQt(loom::asString(*text)));
}

void PlayerWindow::chooseOption(int index)
{
    if (interpreter == nullptr) return;

    clearChoices();

    // The reader shows one passage at a time: what came before is finished with.
    passage->clear();

    interpreter->choose(index);

    if (interpreter->finished())
    {
        passage->append("<p style=\"color:#8a8a8a;\">The story ends here.</p>");
    }
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

void PlayerWindow::saveGame()
{
    if (interpreter == nullptr) return;

    const QString path = QFileDialog::getSaveFileName(
        this, "Save Game", QString(), "Loom save (*.loomsave)");

    if (path.isEmpty()) return;

    QString failure;
    if (!writeFile(path, loom::writeJson(loom::writeSave(interpreter->save())), failure))
    {
        report("Cannot write " + path + ":\n" + failure);
        return;
    }

    showOverlay(settingsOverlay, false);
}

void PlayerWindow::loadGame()
{
    if (interpreter == nullptr) return;

    const QString path = QFileDialog::getOpenFileName(
        this, "Load Game", QString(), "Loom save (*.loomsave)");

    if (path.isEmpty()) return;

    std::string text;
    QString     failure;
    if (!readFile(path, text, failure))
    {
        report("Cannot read " + path + ":\n" + failure);
        return;
    }

    loom::Value document;
    std::string error;
    if (!loom::parseJson(text, document, error))
    {
        report(toQt(error));
        return;
    }

    loom::SaveState state;
    if (!loom::readSave(document, state, error))
    {
        report(toQt(error));
        return;
    }

    clearChoices();
    passage->clear();

    interpreter->restore(state);

    // Re-runs the node that was waiting, which puts its buttons back on screen.
    interpreter->replay();

    showOverlay(settingsOverlay, false);
}
