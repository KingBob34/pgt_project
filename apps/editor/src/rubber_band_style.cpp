#include "rubber_band_style.h"

#include <QPainter>
#include <QStyleOptionRubberBand>

namespace
{
    // Draws one edge with its dashes counted from the viewport's origin.
    void drawEdge(QPainter* painter, QPen pen, QPointF from, QPointF to, qreal phase)
    {
        pen.setDashOffset(phase);

        painter->setPen(pen);
        painter->drawLine(from, to);
    }
}

void RubberBandStyle::drawControl(ControlElement element, const QStyleOption* option,
                                  QPainter* painter, const QWidget* widget) const
{
    const QStyleOptionRubberBand* band = qstyleoption_cast<const QStyleOptionRubberBand*>(option);

    if (element != CE_RubberBand || band == nullptr)
    {
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    const QRectF rect = QRectF(band->rect).normalized();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    painter->fillRect(rect, QColor(255, 255, 255, 24));

    QPen pen(QColor(230, 230, 230));
    pen.setCosmetic(true);
    pen.setDashPattern({ 4.0, 4.0 });

    drawEdge(painter, pen, rect.topLeft(),    rect.topRight(),     rect.left());
    drawEdge(painter, pen, rect.bottomLeft(), rect.bottomRight(),  rect.left());
    drawEdge(painter, pen, rect.topLeft(),    rect.bottomLeft(),   rect.top());
    drawEdge(painter, pen, rect.topRight(),   rect.bottomRight(),  rect.top());

    painter->restore();
}

int RubberBandStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                               QStyleHintReturn* returnData) const
{
    // The platform mask keeps the border only and would clip the fill away.
    if (hint == SH_RubberBand_Mask) return 0;

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}
