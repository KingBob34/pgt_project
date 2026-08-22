#ifndef LOOM_EDITOR_CONNECTION_PAINTER_H
#define LOOM_EDITOR_CONNECTION_PAINTER_H
#include <QPainterPath>

#include <QtNodes/internal/AbstractConnectionPainter.hpp>

class CanvasFaults;

// The curve a wire is drawn along, in the connection's own coordinates. The
// node painter needs the same curve to finish the stretch of it that runs
// under a card, so the shape of a wire is decided here and nowhere else.
QPainterPath wirePath(const QtNodes::ConnectionGraphicsObject& cgo);

// Draws a wire in the colour of the pin it leaves, so a wire and the two ends
// it joins are one colour, and puts a red halo behind it when the last check
// named either of those pins.
class ConnectionPainter : public QtNodes::AbstractConnectionPainter
{
public:
    explicit ConnectionPainter(const CanvasFaults& faults);

    void         paint(QPainter* painter, const QtNodes::ConnectionGraphicsObject& cgo) const override;
    QPainterPath getPainterStroke(const QtNodes::ConnectionGraphicsObject& cgo) const override;

private:
    bool blamed(const QtNodes::ConnectionGraphicsObject& cgo) const;

    const CanvasFaults& faults;
};

#endif //LOOM_EDITOR_CONNECTION_PAINTER_H
