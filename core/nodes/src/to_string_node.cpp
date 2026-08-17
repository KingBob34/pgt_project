#include "nodes_internal.h"

//==============================================================================
//  To String                                                   Conversion
//------------------------------------------------------------------------------
//  Renders any value as the text of it, so a number can reach a text box.
//
//  Inputs
//      in            Flow
//      value         Any
//
//  Outputs
//      out           Flow
//      result        String
//==============================================================================

namespace loom
{
    namespace
    {
        class ToStringNode : public NodeType
        {
        public:
            std::string name()        const override { return "toString"; }
            std::string displayName() const override { return "To String"; }
            std::string category()    const override { return "Conversion"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Any),
                         flowOut(),
                         dataOut("result", "Result", PinType::String) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", toText(context.input("value")));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeToStringNode()
    {
        return std::make_unique<ToStringNode>();
    }
}
