#include "pin_editor.h"

#include <map>
#include <string>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>

#include "loom/value/inspect.h"

namespace
{
    constexpr int    kIntLimit   = 1000000000;
    constexpr double kFloatLimit = 1e9;

    // The port caption sits beside the editor, so an editor is only as wide as
    // the value it holds.
    constexpr int kNumberWidth = 88;
    constexpr int kTextWidth   = 140;

    QWidget* makeBool(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QCheckBox* box = new QCheckBox;
        box->setChecked(loom::asBool(value));

        QObject::connect(box, &QCheckBox::toggled, [changed](bool on) { changed(on); });

        return box;
    }

    QWidget* makeInt(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QSpinBox* box = new QSpinBox;
        box->setFixedWidth(kNumberWidth);
        box->setRange(-kIntLimit, kIntLimit);
        box->setValue(static_cast<int>(loom::asInt(value)));

        QObject::connect(box, &QSpinBox::valueChanged, [changed](int number) { changed(number); });

        return box;
    }

    QWidget* makeFloat(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QDoubleSpinBox* box = new QDoubleSpinBox;
        box->setFixedWidth(kNumberWidth);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setRange(-kFloatLimit, kFloatLimit);
        box->setDecimals(3);
        box->setValue(loom::asFloat(value));

        QObject::connect(box, &QDoubleSpinBox::valueChanged,
                         [changed](double number) { changed(number); });

        return box;
    }

    QWidget* makeString(const loom::PinSpec& pin, const loom::Value& value, const PinChanged& changed)
    {
        QLineEdit* field = new QLineEdit(QString::fromStdString(loom::asString(value)));
        field->setFixedWidth(kTextWidth);
        field->setPlaceholderText(QString::fromStdString(pin.label));

        QObject::connect(field, &QLineEdit::textChanged, [changed](const QString& text)
        {
            changed(text.toStdString());
        });

        return field;
    }

    using Factory = QWidget* (*)(const loom::PinSpec&, const loom::Value&, const PinChanged&);

    // The pin types an author may type into. A type that is not listed has no
    // editor at all and has to be fed by a wire.
    const std::map<std::string, Factory>& editableTypes()
    {
        static const std::map<std::string, Factory> table = {
            { loom::PinType::Bool,   &makeBool   },
            { loom::PinType::Int,    &makeInt    },
            { loom::PinType::Float,  &makeFloat  },
            { loom::PinType::String, &makeString },
        };

        return table;
    }
}

QWidget* makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed)
{
    const auto factory = editableTypes().find(pin.type);

    if (factory == editableTypes().end()) return nullptr;

    return factory->second(pin, value, changed);
}
