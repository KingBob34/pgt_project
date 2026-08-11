#ifndef LOOM_RUNTIME_INTERPRETER_H
#define LOOM_RUNTIME_INTERPRETER_H
#include <map>
#include <string>
#include <vector>

#include "loom/graph/catalog.h"
#include "loom/graph/execution.h"
#include "loom/graph/graph.h"

namespace loom
{
    // One activation of one graph. Jumping replaces the top frame;
    // calling and returning, when they arrive, will push and pop.
    struct Frame
    {
        std::string graphName;
        NodeId      nodeId = 0;
        std::map<std::string, Value> locals;
    };

    // Identifies one output pin's value slot. The graph name is part of the
    // key because node ids are only unique within a graph.
    struct PinRef
    {
        std::string graphName;
        NodeId      node = 0;
        std::string pin;

        bool operator<(const PinRef& other) const;
        bool operator==(const PinRef& other) const;
    };

    // What the story is waiting for, if anything.
    struct Pending
    {
        enum class Kind { None, Click, Choice };

        Kind                     kind = Kind::None;
        std::string              pin;
        std::vector<std::string> optionPins;
    };


    // Everything that changes while a story runs. If it all fits in here,
    // no state has leaked somewhere it should not be.
    struct SaveState
    {
        std::vector<Frame>           callStack;
        std::map<std::string, Value> variables;
        std::map<PinRef, Value>      outputs;
        Pending                      pending;
        bool                         done = true;
    };

    class Interpreter
    {
    public:
        Interpreter(const Project& project, const NodeCatalog& catalog, Host& host);

        void start();
        void resume();            // the player clicked to carry on
        void choose(int index);   // the player picked option index

        // Pushes the pending prompt at the host again, after a restore.
        void replay();


        bool finished() const;
        bool waiting() const;


        SaveState save() const;
        void      restore(const SaveState& state);

    private:
        bool enter(const std::string& graphName);
        bool advance(const std::string& pin);
        void run();

        const Project&     project;
        const NodeCatalog& catalog;
        Host&              host;

        std::vector<Frame>           callStack;
        std::map<std::string, Value> variables;
        std::map<PinRef, Value>      outputs;
        Pending                      pending;
        bool                         done = true;
    };
}

#endif //LOOM_RUNTIME_INTERPRETER_H
