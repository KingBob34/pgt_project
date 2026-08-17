#include "nodes_internal.h"

//==============================================================================
//  Or                                                               Logic
//------------------------------------------------------------------------------
//  True when either side is true.
//
//  Inputs
//      in            Flow
//      left          Bool
//      right         Bool
//
//  Outputs
//      out           Flow
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class OrNode : public NodeType
        {
        public:
            std::string name()        const override { return "or"; }
            std::string displayName() const override { return "Or"; }
            std::string category()    const override { return "Logic"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("left", "Left", PinType::Bool, Value(false)),
                         dataIn("right", "Right", PinType::Bool, Value(false)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result",
                                  context.inputBool("left") || context.inputBool("right"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeOrNode()
    {
        return std::make_unique<OrNode>();
    }
}
