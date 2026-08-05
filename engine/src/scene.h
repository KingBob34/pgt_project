#ifndef SCENE_H
#define SCENE_H
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "nodes/node.h"
#include "value.h"

// One execution flow connection
struct Connection
{
    int from = 0;   // source node id
    std::string fromPin;   // which output pin of the source node
    int to = 0;   // destination node id
};

// Descriptive information about the scene
struct SceneMeta
{
    std::string title;
    std::string author;
};

// The scene-file schema version this build can read
inline constexpr int kSupportedSchemaVersion = 1;

// One loaded scene file: every node, every wire, and the state the game starts with
struct Scene
{
    int schemaVersion = 0;
    SceneMeta meta;

    // variable name -> its value at the start of the game
    std::map<std::string, Value> initialVariables;

    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<Connection> connections;

    // Resolved once at load time
    Node* start = nullptr;
    std::map<int, Node*> nodesById;
    std::map<std::pair<int, std::string>, Node*> flowTargets;

    // Both return nullptr when there is no such node or nothing wired to that pin
    [[nodiscard]] Node* findNode(int id) const;
    [[nodiscard]] Node* nextNode(int fromId, const std::string& fromPin) const;
};

#endif //SCENE_H
