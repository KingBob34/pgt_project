#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H
#include <functional>
#include <map>
#include <memory>
#include <string>
#include "nodes/node.h"

// Creates one empty node of a concrete type.
// Its fields are filled in afterwards
using NodeFactory = std::function<std::unique_ptr<Node>()>;

// Maps the 'type' string used in story files to the class that implements it
const std::map<std::string, NodeFactory>& nodeRegistry();

#endif //NODE_REGISTRY_H
