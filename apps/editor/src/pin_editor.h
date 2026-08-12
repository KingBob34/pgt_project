#ifndef LOOM_EDITOR_PIN_EDITOR_H
#define LOOM_EDITOR_PIN_EDITOR_H
#include <functional>

#include "loom/graph/pin.h"
#include "loom/value/value.h"

class QWidget;

// Carries a pin's new value out of its editor.
using PinChanged = std::function<void(loom::Value)>;

// An editor for one pin, or nullptr for a pin type that has none.
QWidget* makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed);

#endif //LOOM_EDITOR_PIN_EDITOR_H
