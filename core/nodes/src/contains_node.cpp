#include "nodes_internal.h"

//==============================================================================
//  Contains                                                          Lists
//------------------------------------------------------------------------------
//  True when the list holds a value equal to the one given.
//
//  Inputs
//      in            Flow
//      list          List
//      value         Any
//
//  Outputs
//      out           Flow
//      result        Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class ContainsNode : public NodeType
        {
        public:
            std::string name()        const override { return "contains"; }
            std::string displayName() const override { return "Contains"; }
            std::string category()    const override { return "Lists"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("list", "List", PinType::List),
                         dataIn("value", "Value", PinType::Any),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value list = context.input("list");

                if (!isList(list))
                {
                    reportError(context, "Contains",
                                "cannot look inside a " + pinTypeLabel(typeName(list)));
                    return FlowResult::stop();
                }

                context.setOutput("result", listContains(list, context.input("value")));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeContainsNode()
    {
        return std::make_unique<ContainsNode>();
    }
}
