#include "nodes_internal.h"

//==============================================================================
//  Multiply                                                         Maths
//------------------------------------------------------------------------------
//  The two sides multiplied. Whole numbers only.
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
        class MultiplyNode : public NodeType
        {
        public:
            std::string name()        const override { return "multiply"; }
            std::string displayName() const override { return "*"; }
            std::string category()    const override { return "Maths"; }

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
                context.setOutput("result", context.inputInt("left") * context.inputInt("right"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeMultiplyNode()
    {
        return std::make_unique<MultiplyNode>();
    }
}
