#include "nodes_internal.h"

//==============================================================================
//  >=                                                               Logic
//------------------------------------------------------------------------------
//  True when the left side is above the right, or the two are equal.
//
//  Variables carry no declared type, so a comparison between two things that
//  have no order can only be caught here: the fault is reported and the story
//  halts rather than quietly answering false.
//
//  Inputs
//      left          Any       must be a number
//      right         Any       must be a number
//
//  Outputs
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class GreaterOrEqualNode : public NodeType
        {
        public:
            std::string name()        const override { return "greaterOrEqual"; }
            std::string displayName() const override { return ">="; }
            std::string category()    const override { return "Logic"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("left", "Left", PinType::Any),
                         dataIn("right", "Right", PinType::Any),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value left = context.input("left");
                const Value right = context.input("right");

                // Variables carry no declared type, so ordering two things that
                // have no order can only be caught here. A pure node has no
                // branch to send that down, so it says so and answers false.
                if (!isNumber(left) || !isNumber(right))
                {
                    reportError(context, ">=", "cannot order " + pinTypeLabel(typeName(left)) +
                                                " and " + pinTypeLabel(typeName(right)));

                    context.setOutput("result", false);

                    return FlowResult::stop();
                }

                context.setOutput("result", lessThan(right, left) || equals(left, right));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeGreaterOrEqualNode()
    {
        return std::make_unique<GreaterOrEqualNode>();
    }
}
