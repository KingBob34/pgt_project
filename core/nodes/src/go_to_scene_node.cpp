#include "nodes_internal.h"

//==============================================================================
//  Go To Scene                                                       Flow
//------------------------------------------------------------------------------
//  Leaves this graph and carries on in another one. Nothing ever comes back,
//  which is why the node has no outputs at all.
//
//  Inputs
//      in            Flow
//      scene         String    the graph to continue in
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
            std::string category()    const override { return "Flow"; }

            std::vector<PinSpec> pins(int) const override
            {
                return { flowIn(),
                         dataIn("scene", "Scene", PinType::String, Value("")) };
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
