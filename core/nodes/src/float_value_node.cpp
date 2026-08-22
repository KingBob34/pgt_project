#include "nodes_internal.h"

//==============================================================================
//  Float                                                           Values
//------------------------------------------------------------------------------
//  A constant decimal number. It has no flow pins so it never runs: the value the
//  author types sits on the node and is read straight from there.
//
//  Outputs
//      value         Float
//==============================================================================

namespace loom
{
    namespace
    {
        class FloatValueNode : public NodeType
        {
        public:
            std::string name()        const override { return "floatValue"; }
            std::string displayName() const override { return "Float"; }
            std::string category()    const override { return "Values"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                // A constant. With no flow pins it never runs, so the value the
                // author typed stays on the node and is read straight from there.
                return { dataOut("value", "", PinType::Float, Value(0.0)) };
            }
        };
    }

    std::unique_ptr<NodeType> makeFloatValueNode()
    {
        return std::make_unique<FloatValueNode>();
    }
}
