#include "nodes_internal.h"

#include <cmath>

//==============================================================================
//  Floor                                                    Logic & Maths
//------------------------------------------------------------------------------
//  Turns a decimal number into a whole one, downwards. Which of the three to
//  use is the author's choice, never the engine's.
//
//  Inputs
//      in            Flow
//      value         Float
//
//  Outputs
//      out           Flow
//      result        Int
//==============================================================================

namespace loom
{
    namespace
    {
        class FloorNode : public NodeType
        {
        public:
            std::string name()        const override { return "floor"; }
            std::string displayName() const override { return "Floor"; }
            std::string category()    const override { return "Logic & Maths"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Float, Value(0.0)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const double rounded = std::floor(context.inputFloat("value"));
                context.setOutput("result", static_cast<long long>(rounded));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeFloorNode()
    {
        return std::make_unique<FloorNode>();
    }
}
