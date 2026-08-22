#include "nodes_internal.h"

//==============================================================================
//  Comment                                                        Utility
//------------------------------------------------------------------------------
//  A frame drawn round part of the graph, with a title on it. The engine never
//  runs it. Its size is kept on pins like any other value, so a story that has
//  one in it is the same file as a story that does not.
//
//  Inputs
//      text          String
//      width         Int
//      height        Int
//==============================================================================

namespace loom
{
    namespace
    {
        class CommentNode : public NodeType
        {
        public:
            std::string name()        const override { return "comment"; }
            std::string displayName() const override { return "Comment"; }
            std::string category()    const override { return "Utility"; }

            bool isFrame() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                // No connectors of any kind: nothing may be wired to a frame.
                return { labelTextIn("text", "Title", Value("Comment")),
                         dataIn("width",  "Width",  PinType::Int, Value(360)),
                         dataIn("height", "Height", PinType::Int, Value(220)) };
            }
        };
    }

    std::unique_ptr<NodeType> makeCommentNode()
    {
        return std::make_unique<CommentNode>();
    }
}
