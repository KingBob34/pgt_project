#ifndef LOOM_EDITOR_PIN_EDITOR_H
#define LOOM_EDITOR_PIN_EDITOR_H
#include <functional>

#include "loom/graph/pin.h"
#include "loom/value/value.h"

class QWidget;

// An editor for one data input pin, or nullptr for a pin type that has none.
// The callback carries the pin's new value.
QWidget* makePinEditor(const loom::PinSpec& pin, const loom::Value& value,
                       std::function<void(loom::Value)> changed);

#endif //LOOM_EDITOR_PIN_EDITOR_H
