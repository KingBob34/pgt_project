#ifndef LOOM_GRAPH_DIAGNOSTICS_H
#define LOOM_GRAPH_DIAGNOSTICS_H
#include <string>
#include <vector>

#include "loom/graph/node.h"

namespace loom
{
    enum class Severity { Error, Warning };

    // One problem, pointing at where it is.
    struct Diagnostic
    {
        Severity    severity = Severity::Error;
        std::string message;
        std::string graph;
        NodeId      node = 0;   // 0 when the problem is not tied to one node
        std::string pin;   // empty when the problem is not tied to one pin
    };

    class Diagnostics
    {
    public:
        void error  (std::string message, std::string graph, NodeId node = 0, std::string pin = {});
        void warning(std::string message, std::string graph, NodeId node = 0, std::string pin = {});

        bool hasErrors() const;

        const std::vector<Diagnostic>& all() const { return entries; }

    private:
        std::vector<Diagnostic> entries;
    };
}

#endif //LOOM_GRAPH_DIAGNOSTICS_H
