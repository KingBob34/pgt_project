#include "nodes_internal.h"

//==============================================================================
//  Set Variable                                                 Variables
//------------------------------------------------------------------------------
//  Stores a value in one of the globals the story declares.
//
//  Inputs
//      in            Flow
//      name          Variable  the declared global to write; no wire reaches it
//      value         *         what to store, typed as the variable was declared
//
//  Outputs
//      out           Flow
//==============================================================================

namespace loom
{
    namespace
    {
        class SetVariableNode : public NodeType
        {
        public:
            std::string name()        const override { return "setVariable"; }
            std::string displayName() const override { return "Set Variable"; }
            std::string category()    const override { return "Variables"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         variableIn("name", "Variable"),
                         followsIn("value", "Value", "name"),
                         flowOut() };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                context.writeVariable(context.inputString("name"), context.input("value"));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeSetVariableNode()
    {
        return std::make_unique<SetVariableNode>();
    }
}
