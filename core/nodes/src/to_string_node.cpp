#include "nodes_internal.h"

//==============================================================================
//  To String                                                   Conversion
//------------------------------------------------------------------------------
//  Renders any value as the text of it, so a number can reach a text box.
//
//  Inputs
//      value         Any
//
//  Outputs
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

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("value", "Value", PinType::Any),
                         dataOut("result", "Result", PinType::String) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.setOutput("result", toText(context.input("value")));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeToStringNode()
    {
        return std::make_unique<ToStringNode>();
    }
}
