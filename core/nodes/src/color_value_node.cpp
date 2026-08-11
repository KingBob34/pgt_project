#include "nodes_internal.h"

//==============================================================================
//  Color                                                           Values
//------------------------------------------------------------------------------
//  A constant colour. It has no flow pins so it never runs: the value the
//  author types sits on the node and is read straight from there.
//
//  Outputs
//      value         Color
//==============================================================================

namespace loom
{
    namespace
    {
        class ColorValueNode : public NodeType
        {
        public:
            std::string name()        const override { return "colorValue"; }
            std::string displayName() const override { return "Color"; }
            std::string category()    const override { return "Values"; }

            std::vector<PinSpec> pins(int) const override
            {
                // A constant. With no flow pins it never runs, so the value the
                // author typed stays on the node and is read straight from there.
                return { dataOut("value", "", PinType::Color, defaultColor()) };
            }
        };
    }

    std::unique_ptr<NodeType> makeColorValueNode()
    {
        return std::make_unique<ColorValueNode>();
    }
}
