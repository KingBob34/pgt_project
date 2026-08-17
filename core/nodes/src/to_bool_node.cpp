#include "nodes_internal.h"

//==============================================================================
//  To Bool                                                     Conversion
//------------------------------------------------------------------------------
//  A number counts as true when it is not zero; text must read exactly
//  "true" or "false".
//
//  Inputs
//      in            Flow
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
        class ToBoolNode : public NodeType
        {
        public:
            std::string name()        const override { return "toBool"; }
            std::string displayName() const override { return "To Bool"; }
            std::string category()    const override { return "Conversion"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Any),
                         flowOut(),
                         dataOut("result", "Result", PinType::Bool) };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                const Value given = context.input("value");

                if (isBool(given))
                {
                    context.setOutput("result", asBool(given));
                    return FlowResult::continueOn("out");
                }

                if (isInt(given))
                {
                    context.setOutput("result", asInt(given) != 0);
                    return FlowResult::continueOn("out");
                }

                if (isFloat(given))
                {
                    context.setOutput("result", asFloat(given) != 0.0);
                    return FlowResult::continueOn("out");
                }

                if (isString(given))
                {
                    const std::string text = asString(given);

                    if (text == "true" || text == "false")
                    {
                        context.setOutput("result", text == "true");
                        return FlowResult::continueOn("out");
                    }

                    reportError(context, "To Bool", "'" + text + "' is neither true nor false");

                    return FlowResult::stop();
                }

                reportError(context, "To Bool",
                            "cannot read a " + pinTypeLabel(typeName(given)) +
                            " as true or false");

                return FlowResult::stop();
            }
        };
    }

    std::unique_ptr<NodeType> makeToBoolNode()
    {
        return std::make_unique<ToBoolNode>();
    }
}
