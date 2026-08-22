#include "nodes_internal.h"

//==============================================================================
//  Not                                                              Logic
//------------------------------------------------------------------------------
//  Turns true into false and false into true.
//
//  Inputs
//      value         Bool
//
//  Outputs
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
            std::string category()    const override { return "Logic"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("value", "Value", PinType::Bool, Value(false)),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", !context.inputBool("value"));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeNotNode()
    {
        return std::make_unique<NotNode>();
    }
}
