#include "loom/graph/execution.h"
#include "loom/value/inspect.h"

namespace loom
{
    FlowResult FlowResult::continueOn(std::string pin)
    {
        FlowResult result;
        result.kind = Kind::Continue;
        result.pin = std::move(pin);
        return result;
    }

    FlowResult FlowResult::wait(std::string pin)
    {
        FlowResult result;
        result.kind = Kind::Wait;
        result.pin = std::move(pin);
        return result;
    }

    FlowResult FlowResult::choose(std::vector<std::string> pins)
    {
        FlowResult result;
        result.kind = Kind::Choose;
        result.optionPins = std::move(pins);
        return result;
    }

    FlowResult FlowResult::jump(std::string graph)
    {
        FlowResult result;
        result.kind = Kind::Jump;
        result.targetGraph = std::move(graph);
        return result;
    }

    FlowResult FlowResult::stop()
    {
        return FlowResult();
    }

    bool ExecutionContext::inputBool(const std::string& pin) const
    {
        return asBool(input(pin));
    }

    long long ExecutionContext::inputInt(const std::string& pin) const
    {
        return asInt(input(pin));
    }

    double ExecutionContext::inputFloat(const std::string& pin) const
    {
        return asFloat(input(pin));
    }

    std::string ExecutionContext::inputString(const std::string& pin) const
    {
        return asString(input(pin));
    }
}
