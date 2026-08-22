#ifndef LOOM_NODES_INTERNAL_H
#define LOOM_NODES_INTERNAL_H
#include <functional>
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

    // A string input the author writes a sentence into, in a box of its own.
    PinSpec labelTextIn(std::string name, std::string label, Value defaultValue = Value(""));

    // A passage the author writes in the editor, styled and with slots.
    PinSpec proseIn(std::string name, std::string label);

    // Part of the shape of a resizable node. Kept in the file, never shown.
    PinSpec sizeIn(std::string name, int start);

    // An input naming one of the story's declared globals.
    PinSpec variableIn(std::string name, std::string label);

    // Pins whose type is that of the variable chosen on the 'follows' pin.
    PinSpec followsIn (std::string name, std::string label, std::string follows);
    PinSpec followsOut(std::string name, std::string label, std::string follows);

    // Reports a fault to the front end. The engine never repairs what the
    // author wrote, so every fault it can detect leaves through here. The node
    // names itself, so the console cannot call it something the menu does not.
    void reportError(ExecutionContext& context, const NodeType& node, const std::string& detail);

    // Reads the list a node was told to change: the variable named on 'variable',
    // which must be there and must hold a list. False when it is neither,
    // having said which.
    bool readListVariable(ExecutionContext& context, const NodeType& node,
                          std::string& named, Value& held);

    // The shape shared by every ordering test: read 'left' and 'right', answer
    // 'result'. Two things with no order between them are reported and
    // answered false, a pure node having no branch to send a fault down.
    FlowResult orderedBy(ExecutionContext& context, const NodeType& node,
                         const std::function<bool(const Value&, const Value&)>& test);

    // One factory per node type, each defined in its own translation unit.
    std::unique_ptr<NodeType> makeSceneStartNode();
    std::unique_ptr<NodeType> makeBranchNode();
    std::unique_ptr<NodeType> makeGoToSceneNode();
    std::unique_ptr<NodeType> makeEndNode();

    std::unique_ptr<NodeType> makeShowTextNode();
    std::unique_ptr<NodeType> makeShowChoicesNode();

    std::unique_ptr<NodeType> makeGetVariableNode();
    std::unique_ptr<NodeType> makeSetVariableNode();

    std::unique_ptr<NodeType> makeStringValueNode();
    std::unique_ptr<NodeType> makeIntegerValueNode();
    std::unique_ptr<NodeType> makeFloatValueNode();
    std::unique_ptr<NodeType> makeBoolValueNode();

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
    std::unique_ptr<NodeType> makeDivideNode();
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
