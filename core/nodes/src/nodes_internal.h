#ifndef LOOM_NODES_INTERNAL_H
#define LOOM_NODES_INTERNAL_H
#include <memory>
#include <string>
#include <vector>

#include "loom/graph/catalog.h"
#include "loom/value/inspect.h"

namespace loom
{
    // Pin builders, so each node's pins() reads as a list rather than a table.
    PinSpec flowIn (std::string name = "in", std::string label = "");
    PinSpec flowOut(std::string name = "out", std::string label = "");
    PinSpec dataIn (std::string name, std::string label, std::string type, Value defaultValue = Value());
    PinSpec dataOut(std::string name, std::string label, std::string type, Value defaultValue = Value());

    // String inputs that hold prose: a sentence, and a paragraph.
    PinSpec labelTextIn(std::string name, std::string label, Value defaultValue = Value(""));
    PinSpec longTextIn (std::string name, std::string label, Value defaultValue = Value(""));

    // An input naming one of the story's declared globals.
    PinSpec variableIn(std::string name, std::string label);

    // Pins whose type is that of the variable chosen on the 'follows' pin.
    PinSpec followsIn (std::string name, std::string label, std::string follows);
    PinSpec followsOut(std::string name, std::string label, std::string follows);

    // Opaque white: the starting colour of every style pin.
    Value defaultColor();

    // Reports a fault to the front end. The engine never repairs what the
    // author wrote, so every fault it can detect leaves through here.
    void reportError(ExecutionContext& context, const std::string& node, const std::string& detail);

    // One factory per node type, each defined in its own translation unit.
    std::unique_ptr<NodeType> makeSceneStartNode();
    std::unique_ptr<NodeType> makeBranchNode();
    std::unique_ptr<NodeType> makeGoToSceneNode();

    std::unique_ptr<NodeType> makeShowTextNode();
    std::unique_ptr<NodeType> makeShowChoicesNode();

    std::unique_ptr<NodeType> makeGetVariableNode();
    std::unique_ptr<NodeType> makeSetVariableNode();

    std::unique_ptr<NodeType> makeStringValueNode();
    std::unique_ptr<NodeType> makeIntegerValueNode();
    std::unique_ptr<NodeType> makeFloatValueNode();
    std::unique_ptr<NodeType> makeBoolValueNode();
    std::unique_ptr<NodeType> makeColorValueNode();

    std::unique_ptr<NodeType> makeRandomIntegerNode();
    std::unique_ptr<NodeType> makeEqualNode();
    std::unique_ptr<NodeType> makeLessNode();
    std::unique_ptr<NodeType> makeLessOrEqualNode();
    std::unique_ptr<NodeType> makeGreaterNode();
    std::unique_ptr<NodeType> makeGreaterOrEqualNode();
    std::unique_ptr<NodeType> makeFloorNode();
    std::unique_ptr<NodeType> makeCeilNode();
    std::unique_ptr<NodeType> makeRoundNode();
    std::unique_ptr<NodeType> makeToFloatNode();
    std::unique_ptr<NodeType> makeToStringNode();
    std::unique_ptr<NodeType> makeToBoolNode();

    std::unique_ptr<NodeType> makeAddNode();
    std::unique_ptr<NodeType> makeSubtractNode();
    std::unique_ptr<NodeType> makeMultiplyNode();
    std::unique_ptr<NodeType> makeAndNode();
    std::unique_ptr<NodeType> makeOrNode();
    std::unique_ptr<NodeType> makeNotNode();

    std::unique_ptr<NodeType> makeAddToListNode();
    std::unique_ptr<NodeType> makeRemoveFromListNode();
    std::unique_ptr<NodeType> makeContainsNode();
    std::unique_ptr<NodeType> makeListCountNode();

    std::unique_ptr<NodeType> makeCommentNode();
    std::unique_ptr<NodeType> makePrintNode();
}

#endif //LOOM_NODES_INTERNAL_H
