#ifndef LOOM_EDITOR_INLINE_EDIT_H
#define LOOM_EDITOR_INLINE_EDIT_H
#include <functional>

#include <QLineEdit>
#include <QString>

// A one line box that appears over the thing it is renaming. QLineEdit only
// reports a finished edit on Enter, so this one reports it on losing the focus
// as well: clicking anywhere else is the author saying they are done.
class InlineEdit : public QLineEdit
{
    Q_OBJECT

public:
    // 'done' is handed the typed text, once, whichever way the edit ended.
    InlineEdit(const QString& text, std::function<void(const QString&)> done,
               QWidget* parent = nullptr);

Q_SIGNALS:
    // The answer has been given and the box is finished with. Whoever put it
    // up owns it, and this is where it is taken away.
    void finished();

public:
    // A press anywhere else ends the edit. Losing the focus is not enough on
    // its own: a tab bar takes no focus from a click, so dragging the tabs
    // about would otherwise leave the box behind over the wrong one.
    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void finish(bool keep);

    std::function<void(const QString&)> done;
    bool answered = false;
};

#endif //LOOM_EDITOR_INLINE_EDIT_H
