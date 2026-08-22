#include "loom/serialization/graph_io.h"

#include "loom/value/inspect.h"

namespace loom
{
    namespace
    {
        Value writeMeta(const Meta& meta)
        {
            Value out = makeObject();
            objectSet(out, "title", meta.title);
            objectSet(out, "author", meta.author);

            return out;
        }

        Value writeNode(const NodeInstance& node)
        {
            Value out = makeObject();
            objectSet(out, "id", node.id);
            objectSet(out, "type", node.type);

            Value position = makeObject();
            objectSet(position, "x", node.position.x);
            objectSet(position, "y", node.position.y);
            objectSet(out, "position", position);

            // Left out when it carries no information, so the common node stays
            // three lines in the file and a human can still read it.
            if (node.extraPins != 0) objectSet(out, "extraPins", node.extraPins);

            if (!node.pinValues.empty())
            {
                Value values = makeObject();
                for (const auto& entry : node.pinValues) objectSet(values, entry.first, entry.second);

                objectSet(out, "pinValues", values);
            }

            return out;
        }

        Value writeConnection(const Connection& connection)
        {
            Value out = makeObject();
            objectSet(out, "from", connection.from);
            objectSet(out, "fromPin", connection.fromPin);
            objectSet(out, "to", connection.to);
            objectSet(out, "toPin", connection.toPin);

            return out;
        }

        Value writeVariable(const VariableSpec& variable)
        {
            Value out = makeObject();
            objectSet(out, "type", variable.type);
            objectSet(out, "value", variable.value);

            if (!variable.choices.empty())
            {
                Value choices = makeList();
                for (const std::string& choice : variable.choices) listAppend(choices, choice);

                objectSet(out, "choices", choices);
            }

            return out;
        }

        Value writeGraphBody(const Graph& graph)
        {
            Value out = makeObject();
            objectSet(out, "name", graph.name);
            objectSet(out, "meta", writeMeta(graph.meta));

            Value nodes = makeList();
            for (const NodeInstance& node : graph.nodes) listAppend(nodes, writeNode(node));
            objectSet(out, "nodes", nodes);

            Value connections = makeList();
            for (const Connection& connection : graph.connections)
            {
                listAppend(connections, writeConnection(connection));
            }
            objectSet(out, "connections", connections);

            return out;
        }
    }

    Value writeProject(const Project& project)
    {
        Value graphs = makeList();
        for (const Graph& graph : project.graphs) listAppend(graphs, writeGraphBody(graph));

        Value variables = makeObject();
        for (const auto& entry : project.variables)
        {
            objectSet(variables, entry.first, writeVariable(entry.second));
        }

        Value out = makeObject();
        objectSet(out, "schemaVersion", kSchemaVersion);
        objectSet(out, "meta", writeMeta(project.meta));
        objectSet(out, "entry", project.entry);
        objectSet(out, "variables", variables);
        objectSet(out, "graphs", graphs);

        return out;
    }
}
