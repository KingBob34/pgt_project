#include "nodes_internal.h"

//==============================================================================
//  End                                                              Story
//------------------------------------------------------------------------------
//  Stops the story and says which ending the player reached. Without it a
//  story stops by running out of wire, which the editor cannot tell apart
//  from a route the author has not finished yet.
//
//  Inputs
//      in            Flow
//      text          String    what the player is told this ending is
//==============================================================================

namespace loom
{
    namespace
    {
        class EndNode : public NodeType
        {
        public:
            std::string name()        const override { return "end"; }
            std::string displayName() const override { return "End"; }
            std::string category()    const override { return "Story"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         labelTextIn("text", "Ending") };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                Value details = Value::object();
                details["text"] = context.inputString("text");

                context.host().command("ending", details);

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeEndNode()
    {
        return std::make_unique<EndNode>();
    }
}
