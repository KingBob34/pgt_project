#include "loom/serialization/graph_io.h"

namespace loom
{
    namespace
    {
        Value writeMeta(const Meta& meta)
        {
            Value out = Value::object();
            out["title"] = meta.title;
            out["author"] = meta.author;
            return out;
        }

        Value writeNode(const NodeInstance& node)
        {
            Value out = Value::object();
            out["id"] = node.id;
            out["type"] = node.type;

            Value position = Value::object();
            position["x"] = node.position.x;
            position["y"] = node.position.y;
            out["position"] = position;

            // Left out when it carries no information, so the common node stays
            // three lines in the file and a human can still read it.
            if (node.extraPins != 0) out["extraPins"] = node.extraPins;

            if (!node.pinValues.empty())
            {
                Value values = Value::object();
                for (const auto& entry : node.pinValues) values[entry.first] = entry.second;

                out["pinValues"] = values;
            }

            return out;
        }

        Value writeConnection(const Connection& connection)
        {
            Value out = Value::object();
            out["from"] = connection.from;
            out["fromPin"] = connection.fromPin;
            out["to"] = connection.to;
            out["toPin"] = connection.toPin;
            return out;
        }

        Value writeGraphBody(const Graph& graph)
        {
            Value out = Value::object();
            out["name"] = graph.name;
            out["meta"] = writeMeta(graph.meta);

            Value nodes = Value::array();
            for (const NodeInstance& node : graph.nodes) nodes.push_back(writeNode(node));
            out["nodes"] = nodes;

            Value connections = Value::array();
            for (const Connection& connection : graph.connections)
            {
                connections.push_back(writeConnection(connection));
            }
            out["connections"] = connections;

            return out;
        }
    }

    Value writeGraph(const Graph& graph)
    {
        Value out = writeGraphBody(graph);
        out["schemaVersion"] = kSchemaVersion;
        return out;
    }

    Value writeProject(const Project& project)
    {
        Value graphs = Value::array();
        for (const Graph& graph : project.graphs) graphs.push_back(writeGraphBody(graph));

        Value out = Value::object();
        out["schemaVersion"] = kSchemaVersion;
        out["meta"] = writeMeta(project.meta);
        out["entry"] = project.entry;
        out["graphs"] = graphs;

        return out;
    }
}
