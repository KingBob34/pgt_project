#include "nodes_internal.h"

//==============================================================================
//  Get Variable                                                 Variables
//------------------------------------------------------------------------------
//  Reads a global variable, at the moment something asks for its value.
//
//  The variable is picked from the declared globals and no wire reaches that
//  pin, so a name that leads nowhere means the variable was deleted from under
//  it. That is reported, not offered as a second route.
//
//  Inputs
//      name          Variable  the declared global to read; no wire reaches it
//
//  Outputs
//      value         *         its contents, typed as the variable was declared
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

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { variableIn("name", "Variable"),
                         followsOut("value", "Value", "name") };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const std::string name = context.inputString("name");

                Value found;

                if (!context.readVariable(name, found))
                {
                    reportError(context, *this,
                                "there is no variable called '" + name + "'");
                }

                context.setOutput("value", found);

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeGetVariableNode()
    {
        return std::make_unique<GetVariableNode>();
    }
}
