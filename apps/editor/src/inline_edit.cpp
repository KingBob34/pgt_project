#include "inline_edit.h"

#include <utility>

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMetaObject>

namespace
{
    // Opaque, because whatever is being renamed is still drawn underneath and
    // two names in one place read as neither.
    const char* const kBoxStyle =
        "QLineEdit {"
        "  background: #1e1e20;"
        "  color: #f0f0f0;"
        "  border: 1px solid #8aa0bb;"
        "  border-radius: 3px;"
        "  padding: 0px 4px;"
        "}";
}

InlineEdit::InlineEdit(const QString& text, std::function<void(const QString&)> answer,
                       QWidget* parent)
    : QLineEdit(text, parent)
    , done(std::move(answer))
{
    setStyleSheet(kBoxStyle);

    connect(this, &QLineEdit::returnPressed, this, [this] { finish(true); });

    qApp->installEventFilter(this);
}

bool InlineEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress && !answered)
    {
        QWidget* pressed = qobject_cast<QWidget*>(watched);

        if (pressed != this && (pressed == nullptr || !isAncestorOf(pressed))) finish(true);
    }

    return QLineEdit::eventFilter(watched, event);
}

void InlineEdit::focusOutEvent(QFocusEvent* event)
{
    QLineEdit::focusOutEvent(event);

    finish(true);
}

void InlineEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        finish(false);

        event->accept();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void InlineEdit::finish(bool keep)
{
    // Enter is followed by the focus leaving, and both arrive here.
    if (answered) return;

    answered = true;

    qApp->removeEventFilter(this);

    hide();

    // Queued, because this is raised from inside a focus or key event and the
    // answer may rebuild the very thing the box is standing on. Taking that
    // apart while Qt is still delivering an event to it is a crash.
    QMetaObject::invokeMethod(this, [this, keep]
    {
        if (keep && done) done(text().trimmed());

        Q_EMIT finished();
    }, Qt::QueuedConnection);
}
