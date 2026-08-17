#include "nodes_internal.h"

#include <cmath>

//==============================================================================
//  Ceil                                                        Conversion
//------------------------------------------------------------------------------
//  Turns a decimal number into a whole one, upwards. Which of the three to
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
        class CeilNode : public NodeType
        {
        public:
            std::string name()        const override { return "ceil"; }
            std::string displayName() const override { return "Ceil"; }
            std::string category()    const override { return "Conversion"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Float, Value(0.0)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const double rounded = std::ceil(context.inputFloat("value"));
                context.setOutput("result", static_cast<long long>(rounded));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeCeilNode()
    {
        return std::make_unique<CeilNode>();
    }
}
