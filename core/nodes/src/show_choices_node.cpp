#include "nodes_internal.h"

//==============================================================================
//  Show Choices                                              Presentation
//------------------------------------------------------------------------------
//  Offers the player a set of options and suspends the story until one is
//  picked. The author adds and removes options; empty ones are left out, so
//  the buttons the player sees line up with the routes that are taken.
//
//  Inputs
//      in            Flow
//      option0..N    String    the label on each button
//      fontSize      Int
//      color         Color
//
//  Outputs
//      chosen0..N    Flow      taken when that option is picked
//==============================================================================

namespace loom
{
    namespace
    {
        // The one node with a pin count the author controls.
        class ShowChoicesNode : public NodeType
        {
        public:
            std::string name()        const override { return "showChoices"; }
            std::string displayName() const override { return "Show Choices"; }
            std::string category()    const override { return "Presentation"; }

            // One is enough: a single option is a "carry on" button.
            int minExtraPins() const override { return 1; }
            int maxExtraPins() const override { return 12; }

            std::vector<PinSpec> pins(int extraPins) const override
            {
                std::vector<PinSpec> specs;
                specs.push_back(flowIn());

                for (int index = 0; index < extraPins; ++index)
                {
                    specs.push_back(dataIn(optionName(index), optionLabel(index),
                                           PinType::String, Value("")));
                }

                specs.push_back(dataIn("fontSize", "Font Size", PinType::Int, Value(16)));
                specs.push_back(dataIn("color", "Color", PinType::Color, defaultColor()));

                for (int index = 0; index < extraPins; ++index)
                {
                    specs.push_back(flowOut(chosenName(index), optionLabel(index)));
                }

                return specs;
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                std::vector<Option> options;
                std::vector<std::string> pins;

                for (int index = 0; index < maxExtraPins(); ++index)
                {
                    const std::string text = context.inputString(optionName(index));
                    if (text.empty()) continue;

                    options.push_back(Option{ text });
                    pins.push_back(chosenName(index));
                }

                TextStyle style;
                style.fontSize = context.inputInt("fontSize");
                style.color = context.input("color");

                context.host().askChoice(options, style);

                return FlowResult::choose(pins);
            }

        private:
            static std::string optionName(int index)  { return "option" + std::to_string(index); }
            static std::string chosenName(int index)  { return "chosen" + std::to_string(index); }
            static std::string optionLabel(int index) { return "Option " + std::to_string(index + 1); }
        };
    }

    std::unique_ptr<NodeType> makeShowChoicesNode()
    {
        return std::make_unique<ShowChoicesNode>();
    }
}
