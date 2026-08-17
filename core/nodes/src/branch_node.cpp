#include "nodes_internal.h"

//==============================================================================
//  Branch                                                           Story
//------------------------------------------------------------------------------
//  Sends the story down one of two routes according to a true or false value.
//  The only node that turns data back into control flow.
//
//  Inputs
//      in            Flow
//      condition     Bool      the test to make
//
//  Outputs
//      true          Flow      taken when the condition holds
//      false         Flow      taken when it does not
//==============================================================================

namespace loom
{
    namespace
    {
        class BranchNode : public NodeType
        {
        public:
            std::string name()        const override { return "branch"; }
            std::string displayName() const override { return "Branch"; }
            std::string category()    const override { return "Story"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("condition", "Condition", PinType::Bool, Value(false)),
                         flowOut("true", "True"),
                         flowOut("false", "False") };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                return FlowResult::continueOn(context.inputBool("condition") ? "true" : "false");
            }
        };
    }

    std::unique_ptr<NodeType> makeBranchNode()
    {
        return std::make_unique<BranchNode>();
    }
}
