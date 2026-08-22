#ifndef LOOM_RUNTIME_INTERPRETER_H
#define LOOM_RUNTIME_INTERPRETER_H
#include <map>
#include <random>
#include <set>
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

        // Begins at one node instead of at the story's entry point, so an
        // author can try a scene from its middle.
        void startAt(const std::string& graphName, NodeId nodeId);

        void resume();            // the player clicked to carry on
        void choose(int index);   // the player picked option index

        // Pushes the pending prompt at the host again, after a restore.
        void replay();


        bool finished() const;
        bool waiting() const;

        // The variables as they stand, for a front end that shows them.
        const std::map<std::string, Value>& state() const { return variables; }

        // Which node the story is on. Empty and zero before it has begun.
        std::string currentGraph() const;
        NodeId      currentNode() const;


        SaveState save() const;
        void      restore(const SaveState& state);

    private:
        // Everything a run starts from: no frames, no slots, and the variables
        // back at the values the story declares.
        void reset();

        bool enter(const std::string& graphName);

        // A fault the engine itself found, as opposed to one a node reports.
        void report(const std::string& detail) const;
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

        // The pure nodes part way through being worked out, so that a value
        // asked to help work itself out is caught rather than followed.
        mutable std::set<NodeId>     working;

        // The one source of chance in a run. Seeded once per interpreter, so
        // nothing a node does outlives the story being played.
        mutable std::mt19937_64      randomness{ std::random_device{}() };
    };
}

#endif //LOOM_RUNTIME_INTERPRETER_H
