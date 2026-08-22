#include "move_command.h"

#include <utility>

MoveNodesCommand::MoveNodesCommand(QtNodes::AbstractGraphModel& graphModel,
                                   std::vector<QtNodes::NodeId> nodes, QPointF distance)
    : model(graphModel)
    , moved(std::move(nodes))
    , by(distance)
{
    // Named, so the Edit menu says what it is about to take back.
    setText(moved.size() == 1 ? "Move Node" : "Move Nodes");
}

void MoveNodesCommand::shift(double direction)
{
    for (QtNodes::NodeId node : moved)
    {
        const QPointF was =
            model.nodeData(node, QtNodes::NodeRole::Position).value<QPointF>();

        model.setNodeData(node, QtNodes::NodeRole::Position, was + by * direction);
    }
}

void MoveNodesCommand::undo()
{
    shift(-1.0);
}

void MoveNodesCommand::redo()
{
    shift(1.0);
}
