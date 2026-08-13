#ifndef LOOM_EDITOR_PIN_EDITOR_H
#define LOOM_EDITOR_PIN_EDITOR_H
#include <functional>

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

// The editor for one pin. A null widget means the type has none.
PinEditor makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed);

#endif //LOOM_EDITOR_PIN_EDITOR_H
