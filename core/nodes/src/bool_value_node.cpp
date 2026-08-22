#include "nodes_internal.h"

//==============================================================================
//  Bool                                                            Values
//------------------------------------------------------------------------------
//  A constant true or false. It has no flow pins so it never runs: the value the
//  author types sits on the node and is read straight from there.
//
//  Outputs
//      value         Bool
//==============================================================================

namespace loom
{
    namespace
    {
        class BoolValueNode : public NodeType
        {
        public:
            std::string name()        const override { return "boolValue"; }
            std::string displayName() const override { return "Bool"; }
            std::string category()    const override { return "Values"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                // A constant. With no flow pins it never runs, so the value the
                // author typed stays on the node and is read straight from there.
                return { dataOut("value", "", PinType::Bool, Value(false)) };
            }
        };
    }

    std::unique_ptr<NodeType> makeBoolValueNode()
    {
        return std::make_unique<BoolValueNode>();
    }
}
