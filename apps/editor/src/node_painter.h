#ifndef LOOM_EDITOR_NODE_PAINTER_H
#define LOOM_EDITOR_NODE_PAINTER_H
#include <QtNodes/internal/DefaultNodePainter.hpp>

class CanvasFaults;
class NodeAdaptor;

// Draws a node as a titled card: a strip in its category's colour across the
// top, the rows below it, and a red edge when the last check named it.
//
// A connector says two things by its shape and one by its fill: an arrow
// carries flow and a circle carries a value, its colour names the type, and a
// solid one already has a wire on it.
class NodePainter : public QtNodes::DefaultNodePainter
{
public:
    explicit NodePainter(const CanvasFaults& faults);

    void paint(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const override;

private:
    // A comment: a pane of glass with a title on it and nothing else.
    void drawGrip(QPainter* painter, const QRectF& pane) const;
    void drawFrame(QPainter* painter, QtNodes::NodeGraphicsObject& ngo,
                   const NodeAdaptor& adaptor) const;

    void drawCard(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const;
    void drawTitle(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const;

    // Last of the three, so the edge is the same line whatever it runs along.
    void drawBorder(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const;
    void drawPorts(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const;

    // Wires are drawn under the cards, so the stretch of one that reaches a
    // connector set inside a card is hidden. This puts that stretch back.
    void drawWireTails(QPainter* painter, QtNodes::NodeGraphicsObject& ngo) const;

    const CanvasFaults& faults;
};

#endif //LOOM_EDITOR_NODE_PAINTER_H
