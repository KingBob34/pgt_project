#include "nodes_internal.h"

//==============================================================================
//  Divide                                                             Maths
//------------------------------------------------------------------------------
//  The left divided by the right. The result is a Float even though both sides
//  are whole numbers: dropping the fraction silently is how wrong answers get
//  written down. An author who wants a whole number says which way to go by
//  wiring Floor, Ceil or Round to the result, and the graph then says it too.
//
//  Dividing by zero has no answer, so it is reported and the result is left at
//  zero. The author's numbers are never repaired.
//
//  Inputs
//      left          Int
//      right         Int
//
//  Outputs
//      result        Float
//==============================================================================

namespace loom
{
    namespace
    {
        class DivideNode : public NodeType
        {
        public:
            std::string name()        const override { return "divide"; }
            std::string displayName() const override { return "/"; }
            std::string category()    const override { return "Maths"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { dataIn("left", "Left", PinType::Int, Value(0)),
                         dataIn("right", "Right", PinType::Int, Value(1)),
                         dataOut("result", "Result", PinType::Float) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const long long left = context.inputInt("left");
                const long long right = context.inputInt("right");

                if (right == 0)
                {
                    reportError(context, displayName(), "cannot divide by zero");

                    context.setOutput("result", 0.0);

                    return FlowResult::stop();
                }

                context.setOutput("result", static_cast<double>(left) / static_cast<double>(right));

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeDivideNode()
    {
        return std::make_unique<DivideNode>();
    }
}
