#include "scene.h"

Node* Scene::findNode(int id) const
{
    const auto it = nodesById.find(id);
    return it == nodesById.end() ? nullptr : it->second;
}

Node* Scene::nextNode(int fromId, const std::string& fromPin) const
{
    const auto it = flowTargets.find({fromId, fromPin});
    return it == flowTargets.end() ? nullptr : it->second;
}
