#include "graph_model.h"

#include <algorithm>
#include <QtNodes/ConnectionIdUtils>

#include "loom/graph/pin.h"

bool GraphModel::connectionPossible(QtNodes::ConnectionId connectionId) const
{
    if (!nodeExists(connectionId.outNodeId) || !nodeExists(connectionId.inNodeId)) return false;

    const auto withinBounds = [&](QtNodes::PortType portType)
    {
        const QtNodes::NodeId node = QtNodes::getNodeId(portType, connectionId);
        const QtNodes::NodeRole role = portType == QtNodes::PortType::Out
                                     ? QtNodes::NodeRole::OutPortCount
                                     : QtNodes::NodeRole::InPortCount;

        return QtNodes::getPortIndex(portType, connectionId) < nodeData(node, role).toUInt();
    };

    const auto pinType = [&](QtNodes::PortType portType)
    {
        return portData(QtNodes::getNodeId(portType, connectionId),
                        portType,
                        QtNodes::getPortIndex(portType, connectionId),
                        QtNodes::PortRole::DataType)
            .value<QtNodes::NodeDataType>()
            .id.toStdString();
    };

    const auto hasRoom = [&](QtNodes::PortType portType)
    {
        const QtNodes::NodeId node = QtNodes::getNodeId(portType, connectionId);
        const QtNodes::PortIndex index = QtNodes::getPortIndex(portType, connectionId);

        const auto policy = portData(node, portType, index, QtNodes::PortRole::ConnectionPolicyRole)
                                .value<QtNodes::ConnectionPolicy>();

        return policy == QtNodes::ConnectionPolicy::Many
            || connections(node, portType, index).empty();
    };

    if (!withinBounds(QtNodes::PortType::Out) || !withinBounds(QtNodes::PortType::In)) return false;
    if (!hasRoom(QtNodes::PortType::Out) || !hasRoom(QtNodes::PortType::In)) return false;

    // The same rule the loader applies, so a drawn wire always survives a reload.
    return loom::isCompatible(pinType(QtNodes::PortType::Out), pinType(QtNodes::PortType::In));
}

QtNodes::NodeId GraphModel::newNodeId()
{
    return ++lastNodeId;
}

void GraphModel::loadNode(const QJsonObject& nodeJson)
{
    QtNodes::DataFlowGraphModel::loadNode(nodeJson);

    lastNodeId = std::max(lastNodeId, static_cast<QtNodes::NodeId>(nodeJson["id"].toInt()));
}
