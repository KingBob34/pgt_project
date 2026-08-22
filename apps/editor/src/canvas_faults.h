#ifndef LOOM_EDITOR_CANVAS_FAULTS_H
#define LOOM_EDITOR_CANVAS_FAULTS_H
#include <set>
#include <string>
#include <utility>

#include "loom/graph/node.h"

// What the last check complained about, in the scene being edited. The console
// says it in words; this is what lets the canvas say it in colour.
//
// A wire is named by one of its ends, which is enough to find it: a data input
// takes one wire and a flow output carries one.
class CanvasFaults
{
public:
    void clear()
    {
        nodes.clear();
        pins.clear();
    }

    void add(loom::NodeId node, const std::string& pin)
    {
        if (node == 0) return;

        nodes.insert(node);

        if (!pin.empty()) pins.insert({ node, pin });
    }

    bool marks(loom::NodeId node) const
    {
        return nodes.count(node) != 0;
    }

    bool marks(loom::NodeId node, const std::string& pin) const
    {
        return pins.count({ node, pin }) != 0;
    }

private:
    std::set<loom::NodeId>                         nodes;
    std::set<std::pair<loom::NodeId, std::string>> pins;
};

#endif //LOOM_EDITOR_CANVAS_FAULTS_H
