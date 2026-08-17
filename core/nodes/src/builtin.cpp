#include "loom/nodes/builtin.h"

#include "nodes_internal.h"

namespace loom
{
    void registerBuiltinNodes(NodeCatalog& catalog)
    {
        // Registration order is menu order, so each category arrives whole.
        catalog.add(makeSceneStartNode());
        catalog.add(makeShowTextNode());
        catalog.add(makeShowChoicesNode());
        catalog.add(makeBranchNode());
        catalog.add(makeGoToSceneNode());

        catalog.add(makeGetVariableNode());
        catalog.add(makeSetVariableNode());

        catalog.add(makeAddToListNode());
        catalog.add(makeRemoveFromListNode());
        catalog.add(makeContainsNode());
        catalog.add(makeListCountNode());

        catalog.add(makeStringValueNode());
        catalog.add(makeIntegerValueNode());
        catalog.add(makeFloatValueNode());
        catalog.add(makeBoolValueNode());
        catalog.add(makeColorValueNode());
        catalog.add(makeRandomIntegerNode());

        catalog.add(makeAddNode());
        catalog.add(makeSubtractNode());
        catalog.add(makeMultiplyNode());
        catalog.add(makeFloorNode());
        catalog.add(makeCeilNode());
        catalog.add(makeRoundNode());

        catalog.add(makeEqualNode());
        catalog.add(makeLessNode());
        catalog.add(makeLessOrEqualNode());
        catalog.add(makeGreaterNode());
        catalog.add(makeGreaterOrEqualNode());
        catalog.add(makeAndNode());
        catalog.add(makeOrNode());
        catalog.add(makeNotNode());

        catalog.add(makeToFloatNode());
        catalog.add(makeToStringNode());
        catalog.add(makeToBoolNode());

        catalog.add(makeCommentNode());
        catalog.add(makePrintNode());
    }
}
