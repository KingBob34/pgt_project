#include "nodes_internal.h"

//==============================================================================
//  Go To Scene                                                      Story
//------------------------------------------------------------------------------
//  Leaves this graph and carries on in another one. Nothing ever comes back,
//  which is why the node has no outputs at all.
//
//  Inputs
//      in            Flow
//      scene         Scene     the graph to continue in, picked from the story's
//                              own; no wire reaches it, so it cannot be computed
//==============================================================================

namespace loom
{
    namespace
    {
        class GoToSceneNode : public NodeType
        {
        public:
            std::string name()        const override { return "goToScene"; }
            std::string displayName() const override { return "Go To Scene"; }
            std::string category()    const override { return "Story"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         sceneIn("scene", "Scene") };
            }

            FlowResult execute(ExecutionContext& context) const override
            {
                return FlowResult::jump(context.inputString("scene"));
            }
        };
    }

    std::unique_ptr<NodeType> makeGoToSceneNode()
    {
        return std::make_unique<GoToSceneNode>();
    }
}
