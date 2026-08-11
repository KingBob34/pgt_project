#include "nodes_internal.h"

//==============================================================================
//  Integer                                                         Values
//------------------------------------------------------------------------------
//  A constant whole number. It has no flow pins so it never runs: the value the
//  author types sits on the node and is read straight from there.
//
//  Outputs
//      value         Int
//==============================================================================

namespace loom
{
    namespace
    {
        class IntegerValueNode : public NodeType
        {
        public:
            std::string name()        const override { return "integerValue"; }
            std::string displayName() const override { return "Integer"; }
            std::string category()    const override { return "Values"; }

            std::vector<PinSpec> pins(int) const override
            {
                // A constant. With no flow pins it never runs, so the value the
                // author typed stays on the node and is read straight from there.
                return { dataOut("value", "", PinType::Int, Value(0)) };
            }
        };
    }

    std::unique_ptr<NodeType> makeIntegerValueNode()
    {
        return std::make_unique<IntegerValueNode>();
    }
}
