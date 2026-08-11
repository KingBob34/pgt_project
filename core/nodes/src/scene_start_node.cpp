#include "nodes_internal.h"

//==============================================================================
//  Scene Start                                                       Flow
//------------------------------------------------------------------------------
//  Where the story begins. Every graph has exactly one, it cannot be deleted,
//  and execution starts here whenever the graph is entered.
//
//  Outputs
//      out           Flow
//==============================================================================

namespace loom
{
    namespace
    {
        class SceneStartNode : public NodeType
        {
        public:
            std::string name()        const override { return "sceneStart"; }
            std::string displayName() const override { return "Scene Start"; }
            std::string category()    const override { return "Flow"; }
            bool        isEntryPoint() const override { return true; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowOut() };
            }

            FlowResult execute(ExecutionContext&) const override
            {
                return FlowResult::continueOn("out");
            }
        };
    }

    std::unique_ptr<NodeType> makeSceneStartNode()
    {
        return std::make_unique<SceneStartNode>();
    }
}
