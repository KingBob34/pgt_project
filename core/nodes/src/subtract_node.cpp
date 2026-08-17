#include "nodes_internal.h"

//==============================================================================
//  Subtract                                                  Logic & Maths
//------------------------------------------------------------------------------
//  The right side taken from the left. Whole numbers only.
//
//  Inputs
//      in            Flow
//      left          Int
//      right         Int
//
//  Outputs
//      out           Flow
//      result        Int
//==============================================================================

namespace loom
{
    namespace
    {
        class SubtractNode : public NodeType
        {
        public:
            std::string name()        const override { return "subtract"; }
            std::string displayName() const override { return "-"; }
            std::string category()    const override { return "Logic & Maths"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("left", "Left", PinType::Int, Value(0)),
                         dataIn("right", "Right", PinType::Int, Value(0)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", context.inputInt("left") - context.inputInt("right"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeSubtractNode()
    {
        return std::make_unique<SubtractNode>();
    }
}
