#include "nodes_internal.h"

//==============================================================================
//  Set Variable                                                 Variables
//------------------------------------------------------------------------------
//  Stores a value under a name. Names are plain strings and need no prior
//  declaration: writing to a name nobody has used yet creates it.
//
//  Inputs
//      in            Flow
//      name          String    the variable to write
//      value         Any       what to store
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
                         dataIn("name", "Name", PinType::String, Value("")),
                         dataIn("value", "Value", PinType::Any),
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
