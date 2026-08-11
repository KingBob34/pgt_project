#include "graph_document.h"

#include <algorithm>
#include <tuple>
#include <unordered_set>

#include <QJsonObject>
#include <QPointF>

#include <QtNodes/Definitions>

#include "graph_model.h"
#include "node_adaptor.h"

namespace
{
    // The record QtNodes' own loader expects. Pin values are not in it; the
    // adaptor is handed those directly once the node exists.
    QJsonObject nodeRecord(const loom::NodeInstance& node)
    {
        QJsonObject internal;
        internal["model-name"] = QString::fromStdString(node.type);

        QJsonObject position;
        position["x"] = node.position.x;
        position["y"] = node.position.y;

        QJsonObject record;
        record["id"]            = static_cast<qint64>(node.id);
        record["internal-data"] = internal;
        record["position"]      = position;

        return record;
    }
}

GraphDocument::GraphDocument(GraphModel& graphModel, const loom::NodeCatalog& nodeCatalog)
    : model(graphModel)
    , catalog(nodeCatalog)
{
}

void GraphDocument::clear()
{
    // A copy: deleting a node takes it out of the model's own set.
    const std::unordered_set<QtNodes::NodeId> present = model.allNodeIds();

    for (QtNodes::NodeId id : present) model.deleteNode(id);
}

void GraphDocument::reset()
{
    clear();

    sceneName = "scene";
    sceneMeta = loom::Meta();

    for (const loom::NodeType* type : catalog.all())
    {
        if (!type->isEntryPoint()) continue;

        model.addNode(QString::fromStdString(type->name()));
        break;
    }
}

void GraphDocument::open(const loom::Graph& graph)
{
    clear();

    sceneName = graph.name;
    sceneMeta = graph.meta;

    for (const loom::NodeInstance& node : graph.nodes)
    {
        const QtNodes::NodeId id = static_cast<QtNodes::NodeId>(node.id);

        model.loadNode(nodeRecord(node));

        if (NodeAdaptor* adaptor = model.delegateModel<NodeAdaptor>(id)) adaptor->setInstance(node);
    }

    for (const loom::Connection& wire : graph.connections)
    {
        const QtNodes::NodeId from = static_cast<QtNodes::NodeId>(wire.from);
        const QtNodes::NodeId to   = static_cast<QtNodes::NodeId>(wire.to);

        const NodeAdaptor* source = model.delegateModel<NodeAdaptor>(from);
        const NodeAdaptor* target = model.delegateModel<NodeAdaptor>(to);

        if (source == nullptr || target == nullptr) continue;

        const QtNodes::PortIndex fromPin = source->portIndex(QtNodes::PortType::Out, wire.fromPin);
        const QtNodes::PortIndex toPin   = target->portIndex(QtNodes::PortType::In, wire.toPin);

        if (fromPin == QtNodes::InvalidPortIndex || toPin == QtNodes::InvalidPortIndex) continue;

        model.addConnection({ from, fromPin, to, toPin });
    }
}

loom::Graph GraphDocument::graph() const
{
    loom::Graph out;
    out.name = sceneName;
    out.meta = sceneMeta;

    for (QtNodes::NodeId id : model.allNodeIds())
    {
        const NodeAdaptor* adaptor = model.delegateModel<NodeAdaptor>(id);
        if (adaptor == nullptr) continue;

        loom::NodeInstance node = adaptor->instance();
        node.id = static_cast<loom::NodeId>(id);

        const QPointF at = model.nodeData(id, QtNodes::NodeRole::Position).value<QPointF>();
        node.position = { at.x(), at.y() };

        out.nodes.push_back(node);

        for (const QtNodes::ConnectionId& wire : model.allConnectionIds(id))
        {
            if (wire.outNodeId != id) continue;

            const NodeAdaptor* target = model.delegateModel<NodeAdaptor>(wire.inNodeId);
            if (target == nullptr) continue;

            out.connections.push_back({ static_cast<loom::NodeId>(wire.outNodeId),
                                        adaptor->pinName(QtNodes::PortType::Out, wire.outPortIndex),
                                        static_cast<loom::NodeId>(wire.inNodeId),
                                        target->pinName(QtNodes::PortType::In, wire.inPortIndex) });
        }
    }

    // The model stores both in unordered sets; a file that reshuffles itself on
    // every save cannot be read in a diff.
    std::sort(out.nodes.begin(), out.nodes.end(),
              [](const loom::NodeInstance& left, const loom::NodeInstance& right)
              {
                  return left.id < right.id;
              });

    std::sort(out.connections.begin(), out.connections.end(),
              [](const loom::Connection& left, const loom::Connection& right)
              {
                  return std::tie(left.from, left.fromPin, left.to, left.toPin)
                       < std::tie(right.from, right.fromPin, right.to, right.toPin);
              });

    return out;
}
