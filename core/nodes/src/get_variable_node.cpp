#include "nodes_internal.h"

//==============================================================================
//  Get Variable                                                 Variables
//------------------------------------------------------------------------------
//  Reads a global variable. Missing is not a fault but a second route, so the
//  author can answer "you have no key" without any test node.
//
//  Inputs
//      in            Flow
//      name          String    the variable to read
//
//  Outputs
//      out           Flow      taken when the variable exists
//      notFound      Flow      taken when it does not
//      value         Any       its contents; not written on notFound
//==============================================================================

namespace loom
{
    namespace
    {
        class GetVariableNode : public NodeType
        {
        public:
            std::string name()        const override { return "getVariable"; }
            std::string displayName() const override { return "Get Variable"; }
            std::string category()    const override { return "Variables"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("name", "Name", PinType::String, Value("")),
                         flowOut("out", "Found"),
                         flowOut("notFound", "Not Found"),
                         dataOut("value", "Value", PinType::Any) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                Value found;
                if (!context.readVariable(context.inputString("name"), found))
                {
                    return FlowResult::continueOn("notFound");
                }

                context.setOutput("value", found);

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeGetVariableNode()
    {
        return std::make_unique<GetVariableNode>();
    }
}
