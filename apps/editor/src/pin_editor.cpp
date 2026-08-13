#include "pin_editor.h"

#include <map>
#include <string>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>

#include "loom/value/inspect.h"

namespace
{
    constexpr int    kIntLimit   = 1000000000;
    constexpr double kFloatLimit = 1e9;

    // An editor is only as wide as the value it holds; the caption is beside it.
    constexpr int kNumberWidth = 88;
    constexpr int kTextWidth = 140;

    constexpr int kParagraphWidth = 240;
    constexpr int kParagraphRows = 4;

    PinEditor makeBool(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QCheckBox* box = new QCheckBox;
        box->setChecked(loom::asBool(value));

        QObject::connect(box, &QCheckBox::toggled, [changed](bool on) { changed(on); });

        return { box, 1 };
    }

    PinEditor makeInt(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QSpinBox* box = new QSpinBox;
        box->setFixedWidth(kNumberWidth);
        box->setRange(-kIntLimit, kIntLimit);
        box->setValue(static_cast<int>(loom::asInt(value)));

        QObject::connect(box, &QSpinBox::valueChanged, [changed](int number) { changed(number); });

        return { box, 1 };
    }

    PinEditor makeFloat(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QDoubleSpinBox* box = new QDoubleSpinBox;
        box->setFixedWidth(kNumberWidth);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setRange(-kFloatLimit, kFloatLimit);
        box->setDecimals(3);
        box->setValue(loom::asFloat(value));

        QObject::connect(box, &QDoubleSpinBox::valueChanged,
                         [changed](double number) { changed(number); });

        return { box, 1 };
    }

    PinEditor makeString(const loom::PinSpec& pin, const loom::Value& value, const PinChanged& changed)
    {
        const QString text = QString::fromStdString(loom::asString(value));
        const QString hint = QString::fromStdString(pin.label);

        if (pin.longText)
        {
            QPlainTextEdit* box = new QPlainTextEdit(text);
            box->setFixedWidth(kParagraphWidth);
            box->setPlaceholderText(hint);
            box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            QObject::connect(box, &QPlainTextEdit::textChanged, [box, changed]
            {
                changed(box->toPlainText().toStdString());
            });

            return { box, kParagraphRows };
        }

        QLineEdit* field = new QLineEdit(text);
        field->setFixedWidth(kTextWidth);
        field->setPlaceholderText(hint);

        QObject::connect(field, &QLineEdit::textChanged, [changed](const QString& typed)
        {
            changed(typed.toStdString());
        });

        return { field, 1 };
    }

    using Factory = PinEditor (*)(const loom::PinSpec&, const loom::Value&, const PinChanged&);

    // The pin types an author may type into. Anything else is fed by a wire.
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

PinEditor makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed)
{
    const auto factory = editableTypes().find(pin.type);

    if (factory == editableTypes().end()) return {};

    return factory->second(pin, value, changed);
}
