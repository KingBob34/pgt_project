#ifndef LOOM_GRAPH_PROSE_H
#define LOOM_GRAPH_PROSE_H
#include <functional>
#include <string>
#include <vector>

#include "loom/graph/execution.h"
#include "loom/value/value.h"

namespace loom
{
    // A passage as it sits on a prose pin: a list of spans, each of them
    // either words the author typed or a slot standing for one of the node
    // value pins.
    //
    //   { "spans": [ { "text": "You have ", "size": 22 },
    //                { "slot": 0 },
    //                { "text": " coins." } ] }
    //
    // A text span may carry "font", "size" and "color"; left out, each means
    // the reader decides. A slot span carries nothing but the pin it stands
    // for, so rewiring that pin changes what the passage says without the
    // passage itself being touched.
    namespace prose
    {
        // What a slot reads as. The node answers by looking at the pin.
        using SlotText = std::function<std::string(int slot)>;

        // The passage broken into the runs a front end has to draw.
        // Neighbouring spans are not merged: what the author styled apart
        // stays apart.
        std::vector<TextRun> runs(const Value& passage, const SlotText& slotText);

        // The same words with every style dropped, for the pin that hands the
        // passage on as plain data.
        std::string plain(const Value& passage, const SlotText& slotText);

        // A passage holding one unstyled span, which is what a prose pin
        // starts life with.
        Value fromPlain(const std::string& text);
    }
}

#endif //LOOM_GRAPH_PROSE_H
