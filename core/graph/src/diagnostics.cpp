#include "loom/graph/diagnostics.h"

namespace loom
{
    void Diagnostics::error(std::string message, std::string graph, NodeId node, std::string pin)
    {
        entries.push_back({ Severity::Error, std::move(message), std::move(graph), node, std::move(pin) });
    }

    void Diagnostics::warning(std::string message, std::string graph, NodeId node, std::string pin)
    {
        entries.push_back({ Severity::Warning, std::move(message), std::move(graph), node, std::move(pin) });
    }

    bool Diagnostics::hasErrors() const
    {
        for (const Diagnostic& entry : entries)
        {
            if (entry.severity == Severity::Error) return true;
        }

        return false;
    }
}
