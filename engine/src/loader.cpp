#include "loader.h"
#include <fstream>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <vector>
#include "nodes/node_registry.h"

namespace
{
    // Reads the file from disk and parses it as JSON
    Value readJsonFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("cannot open scene file: " + path);
        }

        Value json;
        file >> json;   // throws nlohmann::json::parse_error on malformed JSON
        return json;
    }

    std::unique_ptr<Node> makeNode(const std::string& type)
    {
        const auto found = nodeRegistry().find(type);
        if (found == nodeRegistry().end())
        {
            throw std::runtime_error("unknown node type: '" + type + "'");
        }
        return found->second();
    }

    void readNodes(const Value& json, Scene& scene)
    {
        for (const Value& nodeJson : json.at("nodes"))
        {
            // build node object according to JSON content
            const std::string type = nodeJson.at("type").get<std::string>();
            std::unique_ptr<Node> node = makeNode(type);
            node->id = nodeJson.at("id").get<int>();
            if (scene.nodesById.count(node->id) > 0)
            {
                throw std::runtime_error("duplicate node id: " + std::to_string(node->id));
            }

            // fill in fields of node
            node->position.x = nodeJson.at("position").at("x").get<double>();
            node->position.y = nodeJson.at("position").at("y").get<double>();
            try
            {
                node->readFields(nodeJson);
            }
            catch (const std::exception& error)
            {
                throw std::runtime_error("node " + std::to_string(node->id) +
                    " (" + type + "): " + error.what());
            }

            // add node into graph
            scene.nodesById[node->id] = node.get();
            scene.nodes.push_back(std::move(node));
        }
    }

    void readConnections(const Value& json, Scene& scene)
    {
        for (const Value& connectionJson : json.at("connections"))
        {
            // build wire object according to JSON
            Connection connection;
            connection.from = connectionJson.at("from").get<int>();
            connection.fromPin = connectionJson.at("fromPin").get<std::string>();
            connection.to = connectionJson.at("to").get<int>();

            const std::string label = "connection " + std::to_string(connection.from) +
                "." + connection.fromPin + " -> " + std::to_string(connection.to);
            Node* source = scene.findNode(connection.from);
            Node* target = scene.findNode(connection.to);
            if (source == nullptr)
            {
                throw std::runtime_error(label + ": there is no node with id " + std::to_string(connection.from));
            }
            if (target == nullptr)
            {
                throw std::runtime_error(label + ": there is no node with id " + std::to_string(connection.to));
            }

            const std::vector<std::string> pins = source->outputPins();
            if (std::find(pins.begin(), pins.end(), connection.fromPin) == pins.end())
            {
                throw std::runtime_error(label + ": a " + source->typeName() +
                    " node has no output pin named '" + connection.fromPin + "'");
            }

            const auto key = std::make_pair(connection.from, connection.fromPin);
            if (scene.flowTargets.count(key) > 0)
            {
                throw std::runtime_error(label + ": that pin already has a wire; "
                    "a flow output may only lead to one node");
            }

            scene.flowTargets[key] = target;
            scene.connections.push_back(connection);
        }
    }

    void resolveStart(Scene& scene)
    {
        for (const std::unique_ptr<Node>& node : scene.nodes)
        {
            if (node->typeName() != "start") continue;
            if (scene.start != nullptr)
            {
                throw std::runtime_error("scene has more than one start node (ids " +
                    std::to_string(scene.start->id) + " and " + std::to_string(node->id) + ")");
            }
            scene.start = node.get();
        }
        if (scene.start == nullptr)
        {
            throw std::runtime_error("scene has no start node");
        }
    }

    void findConditionProblems(const Condition& condition, const Scene& scene,
        const std::string& where, std::vector<std::string>& problems)
    {
        if (condition.kind == ConditionKind::Comparison)
        {
            const std::string root = condition.var.substr(0, condition.var.find('.'));
            if (scene.initialVariables.count(root) == 0)
            {
                problems.push_back(where + ": condition reads undeclared variable '" + root + "'");
            }
            return;
        }
        for (const Condition& child : condition.children)
        {
            findConditionProblems(child, scene, where, problems);
        }
    }
}

std::vector<std::string> findProblems(const Scene& scene)
{
    std::vector<std::string> problems;
    for (const std::unique_ptr<Node>& node : scene.nodes)
    {
        const std::string where = "node " + std::to_string(node->id) + " (" + node->typeName() + ")";
        for (const std::string& pin : node->outputPins())
        {
            if (scene.nextNode(node->id, pin) == nullptr)
            {
                problems.push_back(where + ": output pin '" + pin + "' is not connected");
            }
        }
        for (const Condition* condition : node->conditions())
        {
            findConditionProblems(*condition, scene, where, problems);
        }
    }
    return problems;
}

Scene loadScene(const std::string& path)
{
    const Value json = readJsonFile(path);
    Scene scene;
    scene.schemaVersion = json.at("schemaVersion").get<int>();
    if (scene.schemaVersion != kSupportedSchemaVersion)
    {
        throw std::runtime_error("unsupported schema version: " + std::to_string(scene.schemaVersion));
    }

    // read in metadata & initial variables
    scene.meta.title = json.at("meta").at("title").get<std::string>();
    scene.meta.author = json.at("meta").at("author").get<std::string>();
    if (json.contains("variables"))
    {
        for (const auto& [name, value] : json.at("variables").items()){
            scene.initialVariables[name] = value;
        }
    }

    readNodes(json, scene);
    readConnections(json, scene);
    resolveStart(scene);
    return scene;
}
