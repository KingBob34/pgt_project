#include "nodes_internal.h"

//==============================================================================
//  Not                                                       Logic & Maths
//------------------------------------------------------------------------------
//  Turns true into false and false into true.
//
//  Inputs
//      in            Flow
//      value         Bool
//
//  Outputs
//      out           Flow
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class NotNode : public NodeType
        {
        public:
            std::string name()        const override { return "not"; }
            std::string displayName() const override { return "Not"; }
            std::string category()    const override { return "Logic & Maths"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Bool, Value(false)),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", !context.inputBool("value"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeNotNode()
    {
        return std::make_unique<NotNode>();
    }
}
