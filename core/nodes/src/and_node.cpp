#include "nodes_internal.h"

//==============================================================================
//  And                                                       Logic & Maths
//------------------------------------------------------------------------------
//  True only when both sides are true.
//
//  Inputs
//      in            Flow
//      left          Bool
//      right         Bool
//
//  Outputs
//      out           Flow
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
            std::string category()    const override { return "Logic & Maths"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("left", "Left", PinType::Bool, Value(false)),
                         dataIn("right", "Right", PinType::Bool, Value(false)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result",
                                  context.inputBool("left") && context.inputBool("right"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeAndNode()
    {
        return std::make_unique<AndNode>();
    }
}
