#include "nodes_internal.h"

//==============================================================================
//  Contains                                                         Lists
//------------------------------------------------------------------------------
//  True when the list holds a value equal to the one given.
//
//  Inputs
//      list          List
//      value         Any
//
//  Outputs
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

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("list", "List", PinType::List),
                         dataIn("value", "Value", PinType::Any),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value list = context.input("list");

                if (!isList(list))
                {
                    reportError(context, *this,
                                "cannot look inside a " + pinTypeLabel(typeName(list)));

                    context.setOutput("result", false);

                    return FlowResult::stop();
                }

                context.setOutput("result", listContains(list, context.input("value")));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeContainsNode()
    {
        return std::make_unique<ContainsNode>();
    }
}
