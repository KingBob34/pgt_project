#include "nodes_internal.h"

//==============================================================================
//  Random Integer                                                  Values
//------------------------------------------------------------------------------
//  Picks a whole number between Min and Max, both included. A Min above the
//  Max is the author's slip, so it is reported rather than quietly corrected.
//
//  Inputs
//      in            Flow
//      min           Int       lowest number that may come up
//      max           Int       highest number that may come up
//
//  Outputs
//      out           Flow
//      result        Int
//==============================================================================

namespace loom
{
    namespace
    {
        class RandomIntegerNode : public NodeType
        {
        public:
            std::string name()        const override { return "randomInteger"; }
            std::string displayName() const override { return "Random Integer"; }
            std::string category()    const override { return "Values"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("min", "Min", PinType::Int, Value(0)),
                         dataIn("max", "Max", PinType::Int, Value(100)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const long long low = context.inputInt("min");
                const long long high = context.inputInt("max");

                if (low > high)
                {
                    reportError(context, *this,
                                "Min " + std::to_string(low) + " is greater than Max " + std::to_string(high));
                    return FlowResult::stop();
                }

                context.setOutput("result", context.randomInt(low, high));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeRandomIntegerNode()
    {
        return std::make_unique<RandomIntegerNode>();
    }
}
