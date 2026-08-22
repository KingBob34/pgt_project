#ifndef LOOM_QT_CONVERT_H
#define LOOM_QT_CONVERT_H
#include <string>

#include <QColor>
#include <QString>

#include "loom/value/value.h"

// The two crossings every Qt front end makes: engine text to Qt text, and a
// stored colour to a drawn one.
namespace loom::qt
{
    QString toQt(const std::string& text);

    // Channels are stored as fractions of one. Anything that is not a colour
    // reads as an invalid QColor, which draws as the reader's own.
    QColor toColour(const Value& value);
    Value  fromColour(const QColor& colour);
}

#endif //LOOM_QT_CONVERT_H
