#ifndef LOOM_EDITOR_GRAPH_MODEL_H
#define LOOM_EDITOR_GRAPH_MODEL_H
#include <QJsonObject>
#include <QtNodes/DataFlowGraphModel>

// Replaces the two data flow assumptions QtNodes makes: identical pin types,
// and no cycles.
class GraphModel : public QtNodes::DataFlowGraphModel
{
    Q_OBJECT

public:
    using QtNodes::DataFlowGraphModel::DataFlowGraphModel;

    // A story may return to where it has been.
    bool loopsEnabled() const override { return true; }

    // Ids count from one: zero is how a diagnostic says "no node in particular".
    QtNodes::NodeId newNodeId() override;
    void            loadNode(const QJsonObject& nodeJson) override;

    bool connectionPossible(QtNodes::ConnectionId connectionId) const override;

private:
    QtNodes::NodeId lastNodeId = 0;
};

#endif //LOOM_EDITOR_GRAPH_MODEL_H
