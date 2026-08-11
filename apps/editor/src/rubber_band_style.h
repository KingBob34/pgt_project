#ifndef LOOM_EDITOR_RUBBER_BAND_STYLE_H
#define LOOM_EDITOR_RUBBER_BAND_STYLE_H
#include <QProxyStyle>

// The look of the selection box. Belongs on viewport(), which is the widget
// QGraphicsView asks for it.
class RubberBandStyle : public QProxyStyle
{
    Q_OBJECT

public:
    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget) const override;

    int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                  QStyleHintReturn* returnData) const override;
};

#endif //LOOM_EDITOR_RUBBER_BAND_STYLE_H
