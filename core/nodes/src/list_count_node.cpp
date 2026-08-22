#include "nodes_internal.h"

//==============================================================================
//  List Count                                                       Lists
//------------------------------------------------------------------------------
//  How many items the list holds.
//
//  Inputs
//      list          List
//
//  Outputs
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

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("list", "List", PinType::List),
                         dataOut("result", "Result", PinType::Int) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value list = context.input("list");

                if (!isList(list))
                {
                    reportError(context, *this,
                                "cannot count a " + pinTypeLabel(typeName(list)));

                    context.setOutput("result", 0);

                    return FlowResult::stop();
                }

                context.setOutput("result", static_cast<long long>(listSize(list)));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeListCountNode()
    {
        return std::make_unique<ListCountNode>();
    }
}
