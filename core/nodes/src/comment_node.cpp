#include "nodes_internal.h"

//==============================================================================
//  Comment                                                        Utility
//------------------------------------------------------------------------------
//  A note to whoever reads the graph next, drawn on the canvas and ignored by
//  the engine. With no flow pins it can never become part of the story.
//
//  Inputs
//      text          String
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

            std::vector<PinSpec> pins(int) const override
            {
                // No flow pins: a note on the canvas, never part of the story.
                return { dataIn("text", "", PinType::String, Value("")) };
            }
        };
    }

    std::unique_ptr<NodeType> makeCommentNode()
    {
        return std::make_unique<CommentNode>();
    }
}
