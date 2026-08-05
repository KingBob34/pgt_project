#include "runner.h"
#include <stdexcept>
#include <string>

Runner::Runner(const Scene& scene) : scene(scene)
{
    state.variables = scene.initialVariables;
    state.currentNodeId = scene.start->id;
}

Step Runner::advance()
{
    while (true)
    {
        Node* node = scene.findNode(state.currentNodeId);
        if (node == nullptr)
        {
            throw std::runtime_error("the runner is at node " +
                std::to_string(state.currentNodeId) + ", which does not exist");
        }
        const NodeResult result = node->execute(state);

        // ask and stop hand control back to the frontend without moving on
        if (result.kind == NodeResult::Kind::Ask)
        {
            if (result.options.empty())
            {
                throw std::runtime_error("node " + std::to_string(node->id) +
                    " (choice): every option is hidden, the player cannot go on");
            }
            offeredPins = result.pins;
            Step step;
            step.kind = Step::Kind::AskChoice;
            step.options = result.options;
            return step;
        }
        if (result.kind == NodeResult::Kind::Stop)
        {
            Step step;
            step.kind = Step::Kind::Finished;
            step.outcome = result.outcome;
            return step;
        }

        // Continue and Show both leave the node through result.pin.
        Node* next = scene.nextNode(node->id, result.pin);
        if (next == nullptr)
        {
            throw std::runtime_error("node " + std::to_string(node->id) + " (" +
                node->typeName() + "): output pin '" + result.pin + "' is not connected");
        }
        state.currentNodeId = next->id;
        if (result.kind == NodeResult::Kind::Show)
        {
            Step step;
            step.kind = Step::Kind::ShowText;
            step.text = result.text;
            return step;
        }
        // Continue: nothing to show, go round again
    }
}

void Runner::choose(int index)
{
    if (index < 0 || index >= static_cast<int>(offeredPins.size()))
    {
        throw std::runtime_error("there is no option number " + std::to_string(index + 1));
    }
    const std::string pin = offeredPins[index];
    Node* next = scene.nextNode(state.currentNodeId, pin);
    if (next == nullptr)
    {
        throw std::runtime_error("node " + std::to_string(state.currentNodeId) +
            ": output pin '" + pin + "' is not connected");
    }
    state.currentNodeId = next->id;
    offeredPins.clear();
}
