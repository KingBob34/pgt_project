#ifndef LOOM_EDITOR_NODE_METRICS_H
#define LOOM_EDITOR_NODE_METRICS_H
#include <QFont>
#include <QFontMetrics>

// Every size a node on the canvas is built from. They answer to one another --
// a port sits at the middle of its row, and a row is as tall as the editor
// standing on it -- so they are settled in one place rather than a file each.
namespace metrics
{
    inline constexpr int minimumWidth = 120;

    // What DefaultHorizontalNodeGeometry leaves between ports and keeps to
    // itself, so a row here comes out as tall as a row there.
    inline constexpr int portSpacing = 10;

    inline constexpr int rowGap = 6;

    // How far inside the card a connector sits. Ports on the edge look pinned
    // to nothing; inside, they read as part of the row they belong to.
    inline constexpr int portInset = 18;

    // Between the title strip and the first row under it. The base class
    // leaves nothing there, so an editor is drawn against the colour.
    inline constexpr int bodyGap = 5;

    // Between a connector and the name of the pin it belongs to. The two
    // differ because the direction mark stands between them on an input and
    // points away from them on an output.
    inline constexpr int portTextGapIn = 6;
    inline constexpr int portTextGapOut = 2;

    // The fine grid the canvas already draws. A dropped node settles on it, so
    // two nodes placed by eye end up in line with each other.
    inline constexpr int gridStep = 15;

    // Room around the card for the border stroke to sit in. Small on purpose:
    // the rubber band has to swallow this rectangle, not the drawn one.
    inline constexpr int cardMargin = 4;

    // A wire, and the stub of it the card is standing on.
    inline constexpr double wireWidth = 2.0;

    // An editor is only as wide as the value it holds; the caption is beside it.
    inline constexpr int numberWidth = 88;
    inline constexpr int textWidth = 140;
    inline constexpr int variableWidth = 150;

    inline constexpr int labelWidth = 220;
    inline constexpr int labelRows = 3;

    // The passage box is the one an author spends the day in, so it is given
    // room that no other editor on a node gets. One row of it is the band of
    // styling buttons above the text.
    inline constexpr int proseWidth = 640;
    inline constexpr int proseRows = 20;

    inline constexpr int pinButtonWidth = 22;

    inline int rowHeight()
    {
        return QFontMetrics(QFont()).height() + portSpacing;
    }
}

#endif //LOOM_EDITOR_NODE_METRICS_H
