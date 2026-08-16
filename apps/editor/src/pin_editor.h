#ifndef LOOM_EDITOR_PIN_EDITOR_H
#define LOOM_EDITOR_PIN_EDITOR_H
#include <functional>
#include <map>
#include <string>

#include "loom/graph/graph.h"
#include "loom/graph/pin.h"
#include "loom/value/value.h"

class QWidget;

// Carries a pin's new value out of its editor.
using PinChanged = std::function<void(loom::Value)>;

// An editor and how many port rows tall it is.
struct PinEditor
{
    QWidget* widget = nullptr;
    int rows = 1;
};

// The editor for one pin. A null widget means the type has none. The declared
// variables are what a variableName pin offers; other types ignore them.
PinEditor makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed,
                        const std::map<std::string, loom::VariableSpec>& variables = {});

// Narrows an editor to the width a node on the canvas has room for.
void fitToNode(QWidget* editor, const loom::PinSpec& pin);

// Puts a value into an editor that is already on screen, without reporting it
// back. False if the widget is not one this file made.
bool showInEditor(QWidget* editor, const loom::Value& value);

#endif //LOOM_EDITOR_PIN_EDITOR_H
