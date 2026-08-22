#include "nodes_internal.h"

//==============================================================================
//  >=                                                               Logic
//------------------------------------------------------------------------------
//  True when the left side is above the right, or the two are equal.
//
//  Two things with no order between them are reported to the console and
//  answered false: a pure node has no branch to send a fault down.
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
                return orderedBy(context, *this, [](const Value& left, const Value& right)
                {
                    return lessThan(right, left) || equals(left, right);
                });
            }
        };
    }

    std::unique_ptr<NodeType> makeGreaterOrEqualNode()
    {
        return std::make_unique<GreaterOrEqualNode>();
    }
}
