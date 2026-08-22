#include "nodes_internal.h"

//==============================================================================
//  Subtract                                                         Maths
//------------------------------------------------------------------------------
//  The right side taken from the left. Whole numbers only.
//
//  Inputs
//      left          Int
//      right         Int
//
//  Outputs
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
            std::string category()    const override { return "Maths"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("left", "Left", PinType::Int, Value(0)),
                         dataIn("right", "Right", PinType::Int, Value(0)),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", context.inputInt("left") - context.inputInt("right"));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeSubtractNode()
    {
        return std::make_unique<SubtractNode>();
    }
}
