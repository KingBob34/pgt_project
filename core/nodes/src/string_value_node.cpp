#include "nodes_internal.h"

//==============================================================================
//  String                                                          Values
//------------------------------------------------------------------------------
//  A constant string. It has no flow pins so it never runs: the value the
//  author types sits on the node and is read straight from there.
//
//  Outputs
//      value         String
//==============================================================================

namespace loom
{
    namespace
    {
        class StringValueNode : public NodeType
        {
        public:
            std::string name()        const override { return "stringValue"; }
            std::string displayName() const override { return "String"; }
            std::string category()    const override { return "Values"; }

            bool isPure() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                // A constant. With no flow pins it never runs, so the value the
                // author typed stays on the node and is read straight from there.
                return { dataOut("value", "", PinType::String, Value("")) };
            }
        };
    }

    std::unique_ptr<NodeType> makeStringValueNode()
    {
        return std::make_unique<StringValueNode>();
    }
}
