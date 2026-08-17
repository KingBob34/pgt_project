#include "nodes_internal.h"

//==============================================================================
//  To Float                                                    Conversion
//------------------------------------------------------------------------------
//  Widens a whole number into a decimal one. A Bool arrives as 1 or 0.
//
//  Inputs
//      in            Flow
//      value         Any
//
//  Outputs
//      out           Flow
//      result        Float
//==============================================================================

namespace loom
{
    namespace
    {
        class ToFloatNode : public NodeType
        {
        public:
            std::string name()        const override { return "toFloat"; }
            std::string displayName() const override { return "To Float"; }
            std::string category()    const override { return "Conversion"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Any),
                         flowOut(),
                         dataOut("result", "Result", PinType::Float) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value given = context.input("value");

                // asFloat reads a whole number as zero, so each case is its own.
                if (isInt(given))
                {
                    context.setOutput("result", static_cast<double>(asInt(given)));
                    return FlowResult::continueOn("out");
                }

                if (isFloat(given))
                {
                    context.setOutput("result", asFloat(given));
                    return FlowResult::continueOn("out");
                }

                if (isBool(given))
                {
                    context.setOutput("result", asBool(given) ? 1.0 : 0.0);
                    return FlowResult::continueOn("out");
                }

                reportError(context, "To Float",
                            "cannot read a " + pinTypeLabel(typeName(given)) + " as a number");

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeToFloatNode()
    {
        return std::make_unique<ToFloatNode>();
    }
}
