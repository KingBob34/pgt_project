#include "nodes_internal.h"

//==============================================================================
//  Print                                                          Utility
//------------------------------------------------------------------------------
//  Writes a value to the console for the author to inspect. It never reaches
//  the player's text area, so leaving one in a finished story is harmless.
//
//  Inputs
//      in            Flow
//      value         Any
//
//  Outputs
//      out           Flow
//==============================================================================

namespace loom
{
    namespace
    {
        class PrintNode : public NodeType
        {
        public:
            std::string name()        const override { return "print"; }
            std::string displayName() const override { return "Print"; }
            std::string category()    const override { return "Utility"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("value", "Value", PinType::Any),
                         flowOut() };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                Value details = Value::object();
                details["text"] = toText(context.input("value"));

                context.host().command("print", details);

                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makePrintNode()
    {
        return std::make_unique<PrintNode>();
    }
}
