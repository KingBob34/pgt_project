#include "loom/graph/prose.h"

#include "loom/value/inspect.h"

namespace loom
{
    namespace
    {
        const char* const kSpans = "spans";
        const char* const kText  = "text";
        const char* const kSlot  = "slot";
        const char* const kFont  = "font";
        const char* const kSize  = "size";
        const char* const kColor = "color";
        const char* const kBold = "bold";
        const char* const kItalic = "italic";
        const char* const kUnderline = "underline";

        bool flagAt(const Value& span, const char* key)
        {
            const Value* found = objectGet(span, key);

            return found != nullptr && asBool(*found);
        }

        std::string stringAt(const Value& span, const char* key)
        {
            const Value* found = objectGet(span, key);

            return found == nullptr ? std::string() : asString(*found);
        }

        // A span standing for a pin says so; anything else is words.
        bool isSlot(const Value& span, int& slot)
        {
            const Value* found = objectGet(span, kSlot);

            if (found == nullptr || !isNumber(*found)) return false;

            slot = static_cast<int>(asInt(*found));

            return true;
        }
    }

    std::vector<TextRun> prose::runs(const Value& passage, const SlotText& slotText)
    {
        std::vector<TextRun> made;

        const Value* spans = objectGet(passage, kSpans);

        // A pin the author has never opened, or one holding something that is
        // not a passage at all. Either way there is nothing to say.
        if (spans == nullptr) return made;

        for (std::size_t index = 0; index < listSize(*spans); ++index)
        {
            const Value& span = *listAt(*spans, index);

            TextRun run;
            int     slot = 0;

            // What the run says comes from the pin or from the author; how it
            // looks is written the same way either way.
            run.text = isSlot(span, slot) ? (slotText ? slotText(slot) : std::string())
                                          : stringAt(span, kText);

            run.font = stringAt(span, kFont);

            if (const Value* size = objectGet(span, kSize)) run.size = asInt(*size);
            if (const Value* color = objectGet(span, kColor)) run.color = *color;

            run.bold      = flagAt(span, kBold);
            run.italic    = flagAt(span, kItalic);
            run.underline = flagAt(span, kUnderline);

            made.push_back(run);
        }

        return made;
    }

    std::string prose::plain(const Value& passage, const SlotText& slotText)
    {
        std::string words;

        for (const TextRun& run : runs(passage, slotText)) words += run.text;

        return words;
    }

    Value prose::fromPlain(const std::string& text)
    {
        Value span = makeObject();
        objectSet(span, kText, text);

        Value spans = makeList();
        listAppend(spans, span);

        Value passage = makeObject();
        objectSet(passage, kSpans, spans);

        return passage;
    }
}
