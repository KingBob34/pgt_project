#include "nodes_internal.h"

//==============================================================================
//  And                                                              Logic
//------------------------------------------------------------------------------
//  True only when both sides are true.
//
//  Inputs
//      left          Bool
//      right         Bool
//
//  Outputs
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class AndNode : public NodeType
        {
        public:
            std::string name()        const override { return "and"; }
            std::string displayName() const override { return "And"; }
            std::string category()    const override { return "Logic"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("left", "Left", PinType::Bool, Value(false)),
                         dataIn("right", "Right", PinType::Bool, Value(false)),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result",
                                  context.inputBool("left") && context.inputBool("right"));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeAndNode()
    {
        return std::make_unique<AndNode>();
    }
}
