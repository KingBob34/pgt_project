#include "nodes_internal.h"

//==============================================================================
//  Remove From List                                                 Lists
//------------------------------------------------------------------------------
//  Drops the first item equal to the value given. Finding nothing to drop is a
//  second route, not a fault.
//
//  Inputs
//      in            Flow
//      variable      Variable  the list to take from
//      value         Any       what to look for
//
//  Outputs
//      out           Flow      taken when one was dropped
//      notFound      Flow      taken when the list held no such item
//==============================================================================

namespace loom
{
    namespace
    {
        class RemoveFromListNode : public NodeType
        {
        public:
            std::string name()        const override { return "removeFromList"; }
            std::string displayName() const override { return "Remove From List"; }
            std::string category()    const override { return "Lists"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         variableIn("variable", "List"),
                         dataIn("value", "Value", PinType::Any),
                         flowOut("out", "Removed"),
                         flowOut("notFound", "Not Found") };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const std::string named = context.inputString("variable");

                Value held;

                if (!context.readVariable(named, held))
                {
                    reportError(context, "Remove From List",
                                "there is no variable called '" + named + "'");
                    return FlowResult::stop();
                }

                if (!isList(held))
                {
                    reportError(context, "Remove From List",
                                "'" + named + "' holds a " + pinTypeLabel(typeName(held)) +
                                ", not a list");
                    return FlowResult::stop();
                }

                if (!listRemoveFirst(held, context.input("value")))
                {
                    return FlowResult::continueOn("notFound");
                }

                context.writeVariable(named, held);

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeRemoveFromListNode()
    {
        return std::make_unique<RemoveFromListNode>();
    }
}
