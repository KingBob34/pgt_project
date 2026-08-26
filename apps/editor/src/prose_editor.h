#ifndef LOOM_EDITOR_PROSE_EDITOR_H
#define LOOM_EDITOR_PROSE_EDITOR_H
#include <functional>
#include <string>
#include <vector>

#include <QColor>
#include <QString>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QWidget>

#include "loom/value/value.h"

class QComboBox;
class QMenu;
class QTextEdit;
class QToolButton;

// One of the value pins a passage may quote: which pin it is, what to write on
// the chip standing for it, and the colour of what feeds it.
struct ProseSlot
{
    int     index = 0;
    QString label;
    QColor  colour;
};

// What slots the node offers at the moment the author opens the Insert list.
// Asked for afresh each time, so rewiring a pin is reflected without the
// editor being rebuilt.
using ProseSlots = std::function<std::vector<ProseSlot>()>;

// The box a passage is written in: a band of styling controls over a rich text
// area. A quoted pin sits in the text as one indivisible character, so a single
// backspace takes the whole chip away.
class ProseEditor : public QWidget
{
    Q_OBJECT

public:
    ProseEditor(ProseSlots slots, QWidget* parent = nullptr);

    void        setPassage(const loom::Value& passage);
    loom::Value passage() const;

Q_SIGNALS:
    void edited();

private:
    QWidget*     buildBand();
    QMenu*       buildSwatches();
    QToolButton* buildWeight(QWidget* band, const QString& mark, const QString& hint);

    void applyFont(const QString& chosen);
    void applySize(int points);
    void applyColour(const QColor& colour);
    void insertSlot(const ProseSlot& slot);

    // Styling lands on the selection, or on whatever is typed next when there
    // is none, which is how every text editor behaves.
    void restyle(const std::function<void(QTextCharFormat&)>& change);

    // Where the author was last working. The band opens popups, and a popup is
    // a window of its own, so the text area is not focused while one is up.
    QTextCursor working() const;

    // Brings the band in line with the text under the cursor, so what it shows
    // is what the next character would be.
    void followCursor();

    QTextEdit*   text = nullptr;
    QComboBox*   family = nullptr;
    QComboBox*   size = nullptr;
    QToolButton* colourButton = nullptr;
    QToolButton* boldButton = nullptr;
    QToolButton* italicButton = nullptr;
    QToolButton* underlineButton = nullptr;

    QTextCursor mark;
    QColor      inked;

    ProseSlots offer;

    // A passage put in from outside must not read back out as an edit, and the
    // band being brought in line must not read as the author restyling.
    bool loading = false;
    bool syncing = false;
};

#endif //LOOM_EDITOR_PROSE_EDITOR_H
