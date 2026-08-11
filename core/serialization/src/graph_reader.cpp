#include "loom/serialization/graph_io.h"

#include <string>

#include "loom/graph/validate.h"
#include "loom/value/inspect.h"

namespace loom
{
    namespace
    {
        // Coordinates are the one place where 440 and 440.0 mean the same thing:
        // JSON drops a zero fraction, so a saved position comes back as either.
        double readCoordinate(const Value& value)
        {
            return isInt(value) ? static_cast<double>(asInt(value)) : asFloat(value);
        }

        bool checkVersion(const Value& document, Diagnostics& diagnostics)
        {
            const Value* version = objectGet(document, "schemaVersion");
            if (version == nullptr)
            {
                diagnostics.error("the file does not say which format version it is", "");
                return false;
            }

            if (asInt(*version) != kSchemaVersion)
            {
                diagnostics.error("format version " + std::to_string(asInt(*version)) +
                                  " is not supported, this build reads version " +
                                  std::to_string(kSchemaVersion), "");
                return false;
            }

            return true;
        }

        void readMeta(const Value& source, Meta& out)
        {
            if (const Value* title = objectGet(source, "title")) out.title = asString(*title);
            if (const Value* author = objectGet(source, "author")) out.author = asString(*author);
        }

        bool readNode(const Value& record, const std::string& graphName,
                      NodeInstance& out, Diagnostics& diagnostics)
        {
            const Value* id = objectGet(record, "id");
            if (id == nullptr || !isInt(*id))
            {
                diagnostics.error("a node record has no id", graphName);
                return false;
            }
            out.id = static_cast<NodeId>(asInt(*id));

            const Value* type = objectGet(record, "type");
            if (type == nullptr || !isString(*type))
            {
                diagnostics.error("this node record has no type", graphName, out.id);
                return false;
            }
            out.type = asString(*type);

            if (const Value* position = objectGet(record, "position"))
            {
                if (const Value* x = objectGet(*position, "x")) out.position.x = readCoordinate(*x);
                if (const Value* y = objectGet(*position, "y")) out.position.y = readCoordinate(*y);
            }

            if (const Value* extraPins = objectGet(record, "extraPins"))
            {
                out.extraPins = static_cast<int>(asInt(*extraPins));
            }

            if (const Value* values = objectGet(record, "pinValues"))
            {
                for (const std::string& key : objectKeys(*values))
                {
                    if (const Value* stored = objectGet(*values, key)) out.pinValues[key] = *stored;
                }
            }

            return true;
        }

        bool readConnection(const Value& record, const std::string& graphName,
                            Connection& out, Diagnostics& diagnostics)
        {
            const Value* from = objectGet(record, "from");
            const Value* fromPin = objectGet(record, "fromPin");
            const Value* to = objectGet(record, "to");
            const Value* toPin = objectGet(record, "toPin");

            if (from == nullptr || to == nullptr || fromPin == nullptr || toPin == nullptr)
            {
                diagnostics.error("a connection record is missing one of its four ends", graphName);
                return false;
            }

            out.from = static_cast<NodeId>(asInt(*from));
            out.fromPin = asString(*fromPin);
            out.to = static_cast<NodeId>(asInt(*to));
            out.toPin = asString(*toPin);

            return true;
        }

        bool readGraphBody(const Value& document, Graph& out, Diagnostics& diagnostics)
        {
            if (const Value* name = objectGet(document, "name")) out.name = asString(*name);
            if (const Value* meta = objectGet(document, "meta")) readMeta(*meta, out.meta);

            const Value* nodes = objectGet(document, "nodes");
            if (nodes == nullptr || !isList(*nodes))
            {
                diagnostics.error("the graph has no list of nodes", out.name);
                return false;
            }

            for (const Value& record : *nodes)
            {
                NodeInstance node;

                // One unreadable record is not worth abandoning the rest of the
                // file for: the editor should still open what it can.
                if (readNode(record, out.name, node, diagnostics)) out.nodes.push_back(node);
            }

            if (const Value* connections = objectGet(document, "connections"))
            {
                for (const Value& record : *connections)
                {
                    Connection connection;
                    if (readConnection(record, out.name, connection, diagnostics))
                    {
                        out.connections.push_back(connection);
                    }
                }
            }

            return true;
        }
    }

    bool readGraph(const Value& document, const NodeCatalog& catalog,
                   Graph& out, Diagnostics& diagnostics)
    {
        if (!checkVersion(document, diagnostics)) return false;
        if (!readGraphBody(document, out, diagnostics)) return false;

        validate(out, catalog, diagnostics);

        return true;
    }

    bool readProject(const Value& document, const NodeCatalog& catalog,
                     Project& out, Diagnostics& diagnostics)
    {
        if (!checkVersion(document, diagnostics)) return false;

        if (const Value* meta = objectGet(document, "meta")) readMeta(*meta, out.meta);
        if (const Value* entry = objectGet(document, "entry")) out.entry = asString(*entry);

        const Value* graphs = objectGet(document, "graphs");
        if (graphs == nullptr || !isList(*graphs))
        {
            diagnostics.error("the project has no list of graphs", "");
            return false;
        }

        for (const Value& record : *graphs)
        {
            Graph graph;
            if (readGraphBody(record, graph, diagnostics)) out.graphs.push_back(graph);
        }

        validate(out, catalog, diagnostics);

        return true;
    }
}
