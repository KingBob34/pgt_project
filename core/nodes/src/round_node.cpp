#include "nodes_internal.h"

#include <cmath>

//==============================================================================
//  Round                                                    Logic & Maths
//------------------------------------------------------------------------------
//  Turns a decimal number into a whole one, to the nearest. Which of the three to
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
        class RoundNode : public NodeType
        {
        public:
            std::string name()        const override { return "round"; }
            std::string displayName() const override { return "Round"; }
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
                const double rounded = std::round(context.inputFloat("value"));
                context.setOutput("result", static_cast<long long>(rounded));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeRoundNode()
    {
        return std::make_unique<RoundNode>();
    }
}
