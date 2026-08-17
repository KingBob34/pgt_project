#include "nodes_internal.h"

//==============================================================================
//  List Count                                                        Lists
//------------------------------------------------------------------------------
//  How many items the list holds.
//
//  Inputs
//      in            Flow
//      list          List
//
//  Outputs
//      out           Flow
//      result        Int
//==============================================================================

namespace loom
{
    namespace
    {
        class ListCountNode : public NodeType
        {
        public:
            std::string name()        const override { return "listCount"; }
            std::string displayName() const override { return "List Count"; }
            std::string category()    const override { return "Lists"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("list", "List", PinType::List),
                         flowOut(),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value list = context.input("list");

                if (!isList(list))
                {
                    reportError(context, "List Count",
                                "cannot count a " + pinTypeLabel(typeName(list)));
                    return FlowResult::stop();
                }

                context.setOutput("result", static_cast<long long>(listSize(list)));

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeListCountNode()
    {
        return std::make_unique<ListCountNode>();
    }
}
