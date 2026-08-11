#include "loom/nodes/builtin.h"

#include "nodes_internal.h"

namespace loom
{
    void registerBuiltinNodes(NodeCatalog& catalog)
    {
        catalog.add(makeSceneStartNode());
        catalog.add(makeBranchNode());
        catalog.add(makeGoToSceneNode());

        catalog.add(makeShowTextNode());
        catalog.add(makeShowChoicesNode());

        catalog.add(makeGetVariableNode());
        catalog.add(makeSetVariableNode());

        catalog.add(makeStringValueNode());
        catalog.add(makeIntegerValueNode());
        catalog.add(makeFloatValueNode());
        catalog.add(makeBoolValueNode());
        catalog.add(makeColorValueNode());

        catalog.add(makeRandomIntegerNode());
        catalog.add(makeEqualNode());
        catalog.add(makeLessNode());
        catalog.add(makeLessOrEqualNode());
        catalog.add(makeGreaterNode());
        catalog.add(makeGreaterOrEqualNode());
        catalog.add(makeFloorNode());
        catalog.add(makeCeilNode());
        catalog.add(makeRoundNode());

        catalog.add(makeCommentNode());
        catalog.add(makePrintNode());
    }
}
