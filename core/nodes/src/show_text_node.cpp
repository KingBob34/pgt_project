#include "nodes_internal.h"

#include "loom/graph/prose.h"

//==============================================================================
//  Show Text                                                        Story
//------------------------------------------------------------------------------
//  Adds a passage to the player's text area and carries on without waiting.
//  Passages accumulate until a choice is made, so several of these in a row
//  read as one page.
//
//  The passage is written in the editor rather than typed as plain words: the
//  author styles it there, and drops in slots that stand for the value pins.
//  A slot reads as whatever its pin holds, rendered as text, so a list or a
//  number reaches the page the same way a string does.
//
//  Inputs
//      in            Flow
//      textIn        Prose     the passage, styled and with its slots
//      value0..N     Any       what the slots in the passage read
//
//  Outputs
//      out           Flow
//==============================================================================

namespace loom
{
    namespace
    {
        class ShowTextNode : public NodeType
        {
        public:
            std::string name()        const override { return "showText"; }
            std::string displayName() const override { return "Show Text"; }
            std::string category()    const override { return "Story"; }

            bool isResizable() const override { return true; }

            // A passage with no values in it is the ordinary case, so the
            // author starts with one spare pin rather than none.
            int minExtraPins() const override { return 1; }
            int maxExtraPins() const override { return 12; }

            std::vector<PinSpec> pins(int extraPins) const override
            {
                std::vector<PinSpec> specs;

                specs.push_back(flowIn());
                specs.push_back(proseIn("textIn", "Text"));

                for (int index = 0; index < extraPins; ++index)
                {
                    specs.push_back(dataIn(valueName(index), valueLabel(index), PinType::Any));
                }

                specs.push_back(flowOut());

                specs.push_back(sizeIn("width", kLeastWidth));
                specs.push_back(sizeIn("height", kLeastHeight));

                return specs;
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value passage = context.input("textIn");

                // A slot names a pin by position, so rewiring that pin changes
                // what the passage says without the passage being touched.
                const prose::SlotText slotText = [&context](int slot)
                {
                    return toText(context.input(valueName(slot)));
                };

                context.host().showText(prose::runs(passage, slotText));

                return FlowResult::continueOn("out");
            }

        private:
            // The smallest the box is worth having, which is also the size a
            // new one is made at.
            static constexpr int kLeastWidth = 640;
            static constexpr int kLeastHeight = 320;

            static std::string valueName(int index)  { return "value" + std::to_string(index); }
            static std::string valueLabel(int index) { return "Value " + std::to_string(index + 1); }
        };
    }

    std::unique_ptr<NodeType> makeShowTextNode()
    {
        return std::make_unique<ShowTextNode>();
    }
}
