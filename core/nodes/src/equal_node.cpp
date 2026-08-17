#include "nodes_internal.h"

//==============================================================================
//  ==                                                               Logic
//------------------------------------------------------------------------------
//  True when both sides hold the same value. Unlike the ordering tests any two
//  types can be compared for equality, so this node never faults.
//
//  Inputs
//      in            Flow
//      left          Any
//      right         Any
//
//  Outputs
//      out           Flow
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class EqualNode : public NodeType
        {
        public:
            std::string name()        const override { return "equal"; }
            std::string displayName() const override { return "=="; }
            std::string category()    const override { return "Logic"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("left", "Left", PinType::Any),
                         dataIn("right", "Right", PinType::Any),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                // Any two values can be compared for equality, so this one never faults.
                context.setOutput("result", equals(context.input("left"), context.input("right")));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeEqualNode()
    {
        return std::make_unique<EqualNode>();
    }
}
