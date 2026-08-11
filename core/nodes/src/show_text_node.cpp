#include "nodes_internal.h"

//==============================================================================
//  Show Text                                                 Presentation
//------------------------------------------------------------------------------
//  Adds a paragraph to the player's text area and carries on without waiting.
//  Paragraphs accumulate until a choice is made, so several of these in a row
//  read as one passage.
//
//  Inputs
//      in            Flow
//      textIn        String    the words to show
//      fontSize      Int
//      color         Color
//
//  Outputs
//      out           Flow
//      textOut       String    the same words, for a later node to reuse
//
//  Both text pins are labelled "Text" on the canvas: the editor draws inputs
//  and outputs in separate columns, so only the file needs them told apart.
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
            std::string category()    const override { return "Presentation"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("textIn", "Text", PinType::String, Value("")),
                         dataIn("fontSize", "Font Size", PinType::Int, Value(16)),
                         dataIn("color", "Color", PinType::Color, defaultColor()),
                         flowOut(),
                         dataOut("textOut", "Text", PinType::String) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const std::string text = context.inputString("textIn");

                TextStyle style;
                style.fontSize = context.inputInt("fontSize");
                style.color = context.input("color");

                context.host().showText(text, style);

                // The same words often feed a later node, so they leave as data too.
                context.setOutput("textOut", text);

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeShowTextNode()
    {
        return std::make_unique<ShowTextNode>();
    }
}
