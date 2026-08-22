#include "nodes_internal.h"

//==============================================================================
//  Get Variable                                                 Variables
//------------------------------------------------------------------------------
//  Reads a global variable, at the moment something asks for its value.
//
//  The variable is picked from the story's declared globals and no wire can
//  reach that pin, so the one it names is always there: a variable holding
//  zero is not a missing variable. The only way to ask for one that is gone is
//  to delete it from the panel while a node still names it, and that is a
//  fault to report rather than a route the story can take.
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
                    reportError(context, "Get Variable",
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
