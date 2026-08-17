#include "nodes_internal.h"

//==============================================================================
//  <=                                                               Logic
//------------------------------------------------------------------------------
//  True when the left side is below the right, or the two are equal.
//
//  Variables carry no declared type, so a comparison between two things that
//  have no order can only be caught here: the fault is reported and the story
//  halts rather than quietly answering false.
//
//  Inputs
//      in            Flow
//      left          Any       must be a number
//      right         Any       must be a number
//
//  Outputs
//      out           Flow
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class LessOrEqualNode : public NodeType
        {
        public:
            std::string name()        const override { return "lessOrEqual"; }
            std::string displayName() const override { return "<="; }
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
                const Value left = context.input("left");
                const Value right = context.input("right");

                // Variables carry no declared type, so ordering two things that have
                // no order can only be caught here. Name the fault and halt.
                if (!isNumber(left) || !isNumber(right))
                {
                    reportError(context, "<=", "cannot order " + pinTypeLabel(typeName(left)) +
                                                " and " + pinTypeLabel(typeName(right)));
                    return FlowResult::stop();
                }

                context.setOutput("result", lessThan(left, right) || equals(left, right));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeLessOrEqualNode()
    {
        return std::make_unique<LessOrEqualNode>();
    }
}
