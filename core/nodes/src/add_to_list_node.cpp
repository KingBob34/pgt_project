#include "nodes_internal.h"

//==============================================================================
//  Add To List                                                      Lists
//------------------------------------------------------------------------------
//  Puts a value at the end of a list variable, in place.
//
//  Inputs
//      in            Flow
//      variable      Variable  the list to add to
//      value         Any       what to add
//
//  Outputs
//      out           Flow
//==============================================================================

namespace loom
{
    namespace
    {
        class AddToListNode : public NodeType
        {
        public:
            std::string name()        const override { return "addToList"; }
            std::string displayName() const override { return "Add To List"; }
            std::string category()    const override { return "Lists"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         variableIn("variable", "List"),
                         dataIn("value", "Value", PinType::Any),
                         flowOut() };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const std::string named = context.inputString("variable");

                Value held;

                if (!context.readVariable(named, held))
                {
                    reportError(context, "Add To List", "there is no variable called '" + named + "'");
                    return FlowResult::stop();
                }

                if (!isList(held))
                {
                    reportError(context, "Add To List",
                                "'" + named + "' holds a " + pinTypeLabel(typeName(held)) +
                                ", not a list");
                    return FlowResult::stop();
                }

                listAppend(held, context.input("value"));
                context.writeVariable(named, held);

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeAddToListNode()
    {
        return std::make_unique<AddToListNode>();
    }
}
