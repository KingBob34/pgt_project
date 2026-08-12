#include "pin_editor.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include "loom/value/inspect.h"

namespace
{
    constexpr int    kIntLimit   = 1000000000;
    constexpr double kFloatLimit = 1e9;

    double component(const loom::Value& color, const std::string& key)
    {
        const loom::Value* found = loom::objectGet(color, key);
        if (found == nullptr) return 0.0;

        // Components run from 0.0 to 1.0, and JSON drops a zero fraction.
        if (loom::isInt(*found))   return static_cast<double>(loom::asInt(*found));
        if (loom::isFloat(*found)) return loom::asFloat(*found);

        return 0.0;
    }

    QColor toQColor(const loom::Value& color)
    {
        QColor out;
        out.setRgbF(component(color, "r"), component(color, "g"),
                    component(color, "b"), component(color, "a"));

        return out;
    }

    loom::Value fromQColor(const QColor& color)
    {
        loom::Value out = loom::Value::object();
        out["r"] = color.redF();
        out["g"] = color.greenF();
        out["b"] = color.blueF();
        out["a"] = color.alphaF();

        return out;
    }
}

QWidget* makePinEditor(const loom::PinSpec& pin, const loom::Value& value,
                       std::function<void(loom::Value)> changed)
{
    if (pin.type == loom::PinType::Bool)
    {
        QCheckBox* box = new QCheckBox(QString::fromStdString(pin.label));
        box->setChecked(loom::asBool(value));

        QObject::connect(box, &QCheckBox::toggled, [changed](bool on) { changed(on); });

        return box;
    }

    if (pin.type == loom::PinType::Int)
    {
        QSpinBox* box = new QSpinBox;
        box->setRange(-kIntLimit, kIntLimit);
        box->setValue(static_cast<int>(loom::asInt(value)));

        QObject::connect(box, &QSpinBox::valueChanged, [changed](int number) { changed(number); });

        return box;
    }

    if (pin.type == loom::PinType::Float)
    {
        QDoubleSpinBox* box = new QDoubleSpinBox;
        box->setRange(-kFloatLimit, kFloatLimit);
        box->setDecimals(3);
        box->setValue(loom::asFloat(value));

        QObject::connect(box, &QDoubleSpinBox::valueChanged,
                         [changed](double number) { changed(number); });

        return box;
    }

    if (pin.type == loom::PinType::String)
    {
        QLineEdit* field = new QLineEdit(QString::fromStdString(loom::asString(value)));
        field->setPlaceholderText(QString::fromStdString(pin.label));

        QObject::connect(field, &QLineEdit::textChanged, [changed](const QString& text)
        {
            changed(text.toStdString());
        });

        return field;
    }

    if (pin.type == loom::PinType::Color)
    {
        QPushButton* button = new QPushButton;
        button->setFlat(true);

        QColor current = toQColor(value);
        button->setStyleSheet("background-color: " + current.name(QColor::HexArgb));

        QObject::connect(button, &QPushButton::clicked, [button, changed, current]() mutable
        {
            const QColor chosen = QColorDialog::getColor(current, button, "Pin Colour",
                                                         QColorDialog::ShowAlphaChannel);
            if (!chosen.isValid()) return;

            current = chosen;
            button->setStyleSheet("background-color: " + chosen.name(QColor::HexArgb));

            changed(fromQColor(chosen));
        });

        return button;
    }

    // A flow pin is a wire, and an Any pin takes whatever a value node sends
    // it, so neither has anything to type into.
    return nullptr;
}
