#ifndef LOOM_EDITOR_NODE_PALETTE_H
#define LOOM_EDITOR_NODE_PALETTE_H
#include <string>

#include <QColor>

#include "loom/graph/node.h"

// The colours the canvas is drawn in. Kept apart from the painters so that
// what a node looks like and what colour it is stay two separate questions.
namespace palette
{
    // The strip across a node's title. Where a flow begins or ends is marked
    // out from the work in between; everything else takes its category's hue.
    QColor title(const loom::NodeType& type);

    // The body a node's rows sit on, and the line around it.
    QColor body();
    QColor border(bool selected);

    // What the last check complained about.
    QColor fault();

    // A wire being dragged that would connect if it were let go now.
    QColor ready();

    QColor caption();

    // A pin's connector, by the type it carries. Blueprint teaches these once
    // and then a wire can be read without following it to either end.
    QColor pin(const std::string& type);
}

#endif //LOOM_EDITOR_NODE_PALETTE_H
