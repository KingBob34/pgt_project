#include "nodes_internal.h"

//==============================================================================
//  Add                                                       Logic & Maths
//------------------------------------------------------------------------------
//  The two sides added together. Whole numbers only.
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
        class AddNode : public NodeType
        {
        public:
            std::string name()        const override { return "add"; }
            std::string displayName() const override { return "+"; }
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
                context.setOutput("result", context.inputInt("left") + context.inputInt("right"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeAddNode()
    {
        return std::make_unique<AddNode>();
    }
}
