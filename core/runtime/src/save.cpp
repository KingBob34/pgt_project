#include "loom/runtime/save.h"

#include "loom/value/inspect.h"

namespace loom
{
    namespace
    {
        std::string pendingKindName(Pending::Kind kind)
        {
            switch (kind)
            {
                case Pending::Kind::Click:  return "click";
                case Pending::Kind::Choice: return "choice";
                default:                    return "none";
            }
        }

        Pending::Kind pendingKindFromName(const std::string& name)
        {
            if (name == "click")  return Pending::Kind::Click;
            if (name == "choice") return Pending::Kind::Choice;

            return Pending::Kind::None;
        }

        Value writeMap(const std::map<std::string, Value>& entries)
        {
            Value out = Value::object();
            for (const auto& entry : entries) out[entry.first] = entry.second;

            return out;
        }

        void readMap(const Value& source, std::map<std::string, Value>& out)
        {
            out.clear();
            if (!isObject(source)) return;

            for (auto entry = source.begin(); entry != source.end(); ++entry)
            {
                out[entry.key()] = entry.value();
            }
        }
    }

    Value writeSave(const SaveState& state)
    {
        Value frames = Value::array();
        for (const Frame& frame : state.callStack)
        {
            Value record = Value::object();
            record["graph"] = frame.graphName;
            record["node"] = frame.nodeId;
            record["locals"] = writeMap(frame.locals);
            frames.push_back(record);
        }

        // An array, not an object: a slot is keyed by three fields, not by a string.
        Value slots = Value::array();
        for (const auto& slot : state.outputs)
        {
            Value record = Value::object();
            record["graph"] = slot.first.graphName;
            record["node"] = slot.first.node;
            record["pin"] = slot.first.pin;
            record["value"] = slot.second;
            slots.push_back(record);
        }

        Value pending = Value::object();
        pending["kind"] = pendingKindName(state.pending.kind);
        pending["pin"] = state.pending.pin;
        pending["optionPins"] = state.pending.optionPins;

        Value document = Value::object();
        document["saveVersion"] = kSaveVersion;
        document["callStack"] = frames;
        document["variables"] = writeMap(state.variables);
        document["outputs"] = slots;
        document["pending"] = pending;
        document["done"] = state.done;

        return document;
    }

    bool readSave(const Value& document, SaveState& out, std::string& error)
    {
        const Value* version = objectGet(document, "saveVersion");
        if (version == nullptr || asInt(*version) != kSaveVersion)
        {
            error = "unrecognised save file version, expected " + std::to_string(kSaveVersion);
            return false;
        }

        out = SaveState();

        if (const Value* frames = objectGet(document, "callStack"))
        {
            for (const Value& record : *frames)
            {
                Frame frame;

                if (const Value* graph = objectGet(record, "graph")) frame.graphName = asString(*graph);
                if (const Value* node = objectGet(record, "node")) frame.nodeId = static_cast<NodeId>(asInt(*node));
                if (const Value* locals = objectGet(record, "locals")) readMap(*locals, frame.locals);

                out.callStack.push_back(frame);
            }
        }

        if (out.callStack.empty())
        {
            error = "the save file has an empty call stack";
            return false;
        }

        if (const Value* variables = objectGet(document, "variables")) readMap(*variables, out.variables);

        if (const Value* slots = objectGet(document, "outputs"))
        {
            for (const Value& record : *slots)
            {
                PinRef reference;

                if (const Value* graph = objectGet(record, "graph")) reference.graphName = asString(*graph);
                if (const Value* node = objectGet(record, "node")) reference.node = static_cast<NodeId>(asInt(*node));
                if (const Value* pin = objectGet(record, "pin")) reference.pin = asString(*pin);
                if (const Value* value = objectGet(record, "value")) out.outputs[reference] = *value;
            }
        }

        if (const Value* pending = objectGet(document, "pending"))
        {
            if (const Value* kind = objectGet(*pending, "kind")) out.pending.kind = pendingKindFromName(asString(*kind));
            if (const Value* pin = objectGet(*pending, "pin")) out.pending.pin = asString(*pin);

            if (const Value* pins = objectGet(*pending, "optionPins"))
            {
                for (const Value& pin : *pins) out.pending.optionPins.push_back(asString(pin));
            }
        }

        if (const Value* done = objectGet(document, "done")) out.done = asBool(*done);

        error.clear();
        return true;
    }
}
