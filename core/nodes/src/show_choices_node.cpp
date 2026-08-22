#include "nodes_internal.h"

//==============================================================================
//  Show Choices                                                     Story
//------------------------------------------------------------------------------
//  Offers the player a set of options and suspends the story until one is
//  picked. The author adds and removes options; empty ones are left out, so
//  the buttons the player sees line up with the routes that are taken.
//
//  An option the story has not unlocked yet is still shown, greyed and out of
//  reach, so the player can see the route is there.
//
//  Inputs
//      in            Flow
//      option0..N    String    the label on each button
//      enabled0..N   Bool      whether that button can be picked
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
            std::string category()    const override { return "Story"; }

            // One is enough: a single option is a "carry on" button.
            int minExtraPins() const override { return 1; }
            int maxExtraPins() const override { return 12; }

            std::vector<PinSpec> pins(int extraPins) const override
            {
                std::vector<PinSpec> specs;
                specs.push_back(flowIn());

                // Each option is followed by the switch that governs it, so the
                // two read as one row of the node.
                for (int index = 0; index < extraPins; ++index)
                {
                    specs.push_back(labelTextIn(optionName(index), optionLabel(index)));
                    specs.push_back(dataIn(enabledName(index), enabledLabel(index),
                                           PinType::Bool, Value(true)));
                }

                for (int index = 0; index < extraPins; ++index)
                {
                    specs.push_back(flowOut(chosenName(index), optionLabel(index)));
                }

                return specs;
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                std::vector<Option>      options;
                std::vector<std::string> pins;

                // A running node is not told how many options it was given, so
                // every pin it could have is read. One that is not there reads
                // as empty, which is also how an option the author left blank
                // reads, and both are left out.
                for (int index = 0; index < maxExtraPins(); ++index)
                {
                    const std::string text = context.inputString(optionName(index));
                    if (text.empty()) continue;

                    options.push_back(Option{ text, context.inputBool(enabledName(index)) });
                    pins.push_back(chosenName(index));
                }

                context.host().askChoice(options);

                return FlowResult::choose(pins);
            }

        private:
            static std::string optionName(int index)  { return "option" + std::to_string(index); }
            static std::string enabledName(int index) { return "enabled" + std::to_string(index); }
            static std::string chosenName(int index)  { return "chosen" + std::to_string(index); }

            static std::string optionLabel(int index)  { return "Option " + std::to_string(index + 1); }
            static std::string enabledLabel(int index) { return "Enabled " + std::to_string(index + 1); }
        };
    }

    std::unique_ptr<NodeType> makeShowChoicesNode()
    {
        return std::make_unique<ShowChoicesNode>();
    }
}
