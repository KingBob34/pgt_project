#ifndef LOOM_EDITOR_MOVE_COMMAND_H
#define LOOM_EDITOR_MOVE_COMMAND_H
#include <vector>

#include <QPointF>
#include <QUndoCommand>

#include <QtNodes/AbstractGraphModel>

// Moving nodes, as one step of the undo history. QtNodes has a command of its
// own for this, but it reads the selection at the moment it is undone rather
// than holding on to the nodes that actually moved, and it merges with the
// move before it, so a run of drags collapses into one step.
class MoveNodesCommand : public QUndoCommand
{
public:
    MoveNodesCommand(QtNodes::AbstractGraphModel& model,
                     std::vector<QtNodes::NodeId> moved, QPointF by);

    void undo() override;
    void redo() override;

private:
    void shift(double direction);

    QtNodes::AbstractGraphModel& model;
    std::vector<QtNodes::NodeId> moved;
    QPointF                      by;
};

#endif //LOOM_EDITOR_MOVE_COMMAND_H
