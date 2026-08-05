#include "nodes/node_registry.h"
#include "nodes/start_node.h"
#include "nodes/narrative_node.h"
#include "nodes/choice_node.h"
#include "nodes/condition_node.h"
#include "nodes/end_node.h"

namespace
{
    std::map<std::string, NodeFactory> buildRegistry()
    {
        std::map<std::string, NodeFactory> registry;
        registry["start"] = [] {return std::make_unique<StartNode>();};
        registry["narrative"] = [] {return std::make_unique<NarrativeNode>();};
        registry["choice"] = [] {return std::make_unique<ChoiceNode>();};
        registry["condition"] = [] {return std::make_unique<ConditionNode>();};
        registry["end"] = [] {return std::make_unique<EndNode>();};
        return registry;
    }
}

const std::map<std::string, NodeFactory>& nodeRegistry()
{
    static const std::map<std::string, NodeFactory> registry = buildRegistry();
    return registry;
}
