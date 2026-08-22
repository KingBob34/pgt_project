#include "playtest_panel.h"

#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "loom/qt/convert.h"
#include "loom/qt/passage.h"
#include "loom/value/inspect.h"

using loom::qt::toQt;

namespace
{
    // Tighter than the game's, because the panel shares the window with the
    // canvas and never gets the width a reader would.
    const char* const kOptionStyle =
        "QPushButton {"
        "  color: #1a1a1a;"
        "  background: #f2f2f2;"
        "  border: 1px solid #b4b4b4;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  text-align: left;"
        "}"
        "QPushButton:hover { background: #e4e4e4; border-color: #7a7a7a; }"
        "QPushButton:pressed { background: #d2d2d2; }"
        "QPushButton:disabled { color: #a0a0a0; background: #ececec; border-color: #d8d8d8; }";
}

PlaytestPanel::PlaytestPanel(const loom::NodeCatalog& nodeCatalog, QWidget* parent)
    : QWidget(parent)
    , catalog(nodeCatalog)
{
    passage = new QTextEdit;
    passage->setReadOnly(true);
    passage->setFrameStyle(QFrame::NoFrame);
    passage->setStyleSheet("QTextEdit { background: white; padding: 16px; }");

    choiceRow = new QVBoxLayout;
    choiceRow->setContentsMargins(16, 8, 16, 16);
    choiceRow->setSpacing(6);

    choices = new QWidget;
    choices->setLayout(choiceRow);

    // Named, or a bare QWidget selector would repaint every button in here.
    choices->setObjectName("choiceBar");
    choices->setAttribute(Qt::WA_StyledBackground, true);
    choices->setStyleSheet("#choiceBar { background: white; }");

    QVBoxLayout* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(passage, 1);
    column->addWidget(choices, 0);
}

PlaytestPanel::~PlaytestPanel() = default;

void PlaytestPanel::begin(const loom::Project& source)
{
    clearChoices();
    passage->clear();

    project = source;
    interpreter = std::make_unique<loom::Interpreter>(project, catalog, *this);
}

void PlaytestPanel::play(const loom::Project& source)
{
    begin(source);

    interpreter->start();
}

void PlaytestPanel::playFrom(const loom::Project& source, const std::string& graph,
                             loom::NodeId node)
{
    begin(source);

    interpreter->startAt(graph, node);
}

void PlaytestPanel::stop()
{
    interpreter.reset();

    clearChoices();
    passage->clear();
}

void PlaytestPanel::showText(const std::vector<loom::TextRun>& runs)
{
    passage->append(loom::qt::passageHtml(runs));
}

void PlaytestPanel::askChoice(const std::vector<loom::Option>& options)
{
    clearChoices();

    for (std::size_t index = 0; index < options.size(); ++index)
    {
        QPushButton* button = new QPushButton(toQt(options[index].text));
        button->setStyleSheet(kOptionStyle);

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

void PlaytestPanel::command(const std::string& name, const loom::Value& args)
{
    // The author is testing, so an ending goes into the passage rather than
    // interrupting with the card the game puts up.
    if (name == "ending")
    {
        const loom::Value* text = loom::objectGet(args, "text");

        passage->append(QString("<p style=\"color:#8a8a8a;\">--- %1 ---</p>")
                            .arg(text == nullptr ? QString() : toQt(loom::asString(*text))));
        return;
    }

    // Anything else is an extension this front end does not implement.
    if (name != "error") return;

    const loom::Value* node = loom::objectGet(args, "node");
    const loom::Value* detail = loom::objectGet(args, "detail");

    const QString text = QString("%1: %2")
                             .arg(node == nullptr ? QString("engine") : toQt(loom::asString(*node)))
                             .arg(detail == nullptr ? QString() : toQt(loom::asString(*detail)));

    // Raised while a node was running, so that node is the one to blame.
    Q_EMIT faulted(text, toQt(interpreter->currentGraph()), interpreter->currentNode());
}

void PlaytestPanel::chooseOption(int index)
{
    if (interpreter == nullptr) return;

    clearChoices();

    // The reader shows one passage at a time: what came before is finished with.
    passage->clear();

    interpreter->choose(index);
}

void PlaytestPanel::clearChoices()
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
