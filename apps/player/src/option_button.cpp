#include "option_button.h"

#include <algorithm>
#include <cmath>

#include <QFontMetricsF>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QTextLayout>
#include <QTextOption>

namespace
{
    // The padding named in the button's own style sheet. Kept here as well
    // because the text is laid out rather than handed to the style.
    constexpr double kSideGap = 18.0;
    constexpr double kTopGap = 10.0;

    // A little air between wrapped lines: two lines set solid read as a block
    // rather than as one option.
    constexpr double kLineSpacing = 1.25;
}

OptionButton::OptionButton(const QString& label, int fontSize, QWidget* parent)
    : QPushButton(label, parent)
{
    QFont chosen = font();
    chosen.setPointSize(fontSize);

    setFont(chosen);

    QSizePolicy growsDown = sizePolicy();
    growsDown.setHeightForWidth(true);

    setSizePolicy(growsDown);
}

double OptionButton::textWidth(int width) const
{
    return std::max(1.0, width - 2.0 * kSideGap);
}

double OptionButton::layOut(QTextLayout& lines, double width) const
{
    const QFontMetricsF metrics(font());
    const double step = metrics.height() * kLineSpacing;

    QTextOption wrapping;
    wrapping.setWrapMode(QTextOption::WordWrap);

    lines.setFont(font());
    lines.setTextOption(wrapping);

    double used = 0.0;

    lines.beginLayout();

    while (true)
    {
        QTextLine line = lines.createLine();
        if (!line.isValid()) break;

        line.setLineWidth(width);
        line.setPosition(QPointF(0.0, used));

        used += step;
    }

    lines.endLayout();

    return used;
}

int OptionButton::heightForWidth(int width) const
{
    QTextLayout lines(text());

    return static_cast<int>(std::ceil(layOut(lines, textWidth(width)) + 2.0 * kTopGap));
}

QSize OptionButton::sizeHint() const
{
    // On the width it already has, which is the one the column gave it.
    return QSize(QPushButton::sizeHint().width(), heightForWidth(width()));
}

QSize OptionButton::minimumSizeHint() const
{
    return QSize(0, heightForWidth(width()));
}

void OptionButton::resizeEvent(QResizeEvent* event)
{
    QPushButton::resizeEvent(event);

    // A narrower button needs more lines, and the column has to be told.
    updateGeometry();
}

void OptionButton::paintEvent(QPaintEvent*)
{
    QStylePainter painter(this);

    QStyleOptionButton frame;
    initStyleOption(&frame);

    // The style draws everything but the label, which is laid out here.
    frame.text.clear();
    painter.drawControl(QStyle::CE_PushButton, frame);

    QTextLayout lines(text());
    const double used = layOut(lines, textWidth(width()));

    painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,
                                   QPalette::ButtonText));

    lines.draw(&painter, QPointF(kSideGap, (height() - used) / 2.0));
}
