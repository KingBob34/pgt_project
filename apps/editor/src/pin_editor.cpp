#include "pin_editor.h"

#include <map>
#include <string>

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>

#include "loom/value/inspect.h"

namespace
{
    constexpr int    kIntLimit   = 1000000000;
    constexpr double kFloatLimit = 1e9;

    // An editor is only as wide as the value it holds; the caption is beside it.
    constexpr int kNumberWidth = 88;
    constexpr int kTextWidth = 140;
    constexpr int kVariableWidth = 150;

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
        box->setRange(-kIntLimit, kIntLimit);
        box->setValue(static_cast<int>(loom::asInt(value)));

        QObject::connect(box, &QSpinBox::valueChanged, [changed](int number) { changed(number); });

        return { box, 1 };
    }

    PinEditor makeFloat(const loom::PinSpec&, const loom::Value& value, const PinChanged& changed)
    {
        QDoubleSpinBox* box = new QDoubleSpinBox;
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
            box->setPlaceholderText(hint);
            box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            QObject::connect(box, &QPlainTextEdit::textChanged, [box, changed]
            {
                changed(box->toPlainText().toStdString());
            });

            return { box, kParagraphRows };
        }

        QLineEdit* field = new QLineEdit(text);
        field->setPlaceholderText(hint);

        QObject::connect(field, &QLineEdit::textChanged, [changed](const QString& typed)
        {
            changed(typed.toStdString());
        });

        return { field, 1 };
    }

    // A name the story no longer declares is still shown, so the author sees
    // what the node is asking for rather than a blank.
    PinEditor makeVariable(const loom::Value& value, const PinChanged& changed,
                           const std::map<std::string, loom::VariableSpec>& variables)
    {
        const QString chosen = QString::fromStdString(loom::asString(value));

        QComboBox* box = new QComboBox;
        box->addItem(QString(), QString());

        // Every declared variable and every field nested in one, so a node can
        // reach into a group without a walk of its own.
        for (const std::string& path : loom::variablePaths(variables))
        {
            const QString name = QString::fromStdString(path);
            const QString type = QString::fromStdString(loom::declaredTypeAt(variables, path));

            box->addItem(name + "  (" + type + ")", name);
        }

        if (!chosen.isEmpty() && box->findData(chosen) < 0) box->addItem(chosen + " (missing)", chosen);

        box->setCurrentIndex(std::max(0, box->findData(chosen)));

        QObject::connect(box, &QComboBox::currentIndexChanged, box, [box, changed](int)
        {
            changed(box->currentData().toString().toStdString());
        });

        return { box, 1 };
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

PinEditor makePinEditor(const loom::PinSpec& pin, const loom::Value& value, PinChanged changed,
                        const std::map<std::string, loom::VariableSpec>& variables)
{
    if (pin.type == loom::PinType::VariableName) return makeVariable(value, changed, variables);

    const auto factory = editableTypes().find(pin.type);

    if (factory == editableTypes().end()) return {};

    return factory->second(pin, value, changed);
}

void fitToNode(QWidget* editor, const loom::PinSpec& pin)
{
    if (editor == nullptr) return;

    if (pin.type == loom::PinType::VariableName)
    {
        editor->setFixedWidth(kVariableWidth);
        return;
    }

    if (pin.type == loom::PinType::String)
    {
        editor->setFixedWidth(pin.longText ? kParagraphWidth : kTextWidth);
        return;
    }

    if (pin.type == loom::PinType::Int || pin.type == loom::PinType::Float)
    {
        editor->setFixedWidth(kNumberWidth);
    }
}

bool showInEditor(QWidget* editor, const loom::Value& value)
{
    if (editor == nullptr) return false;

    const QSignalBlocker quiet(editor);

    if (QCheckBox* box = qobject_cast<QCheckBox*>(editor))
    {
        box->setChecked(loom::asBool(value));
        return true;
    }

    if (QSpinBox* box = qobject_cast<QSpinBox*>(editor))
    {
        box->setValue(static_cast<int>(loom::asInt(value)));
        return true;
    }

    if (QDoubleSpinBox* box = qobject_cast<QDoubleSpinBox*>(editor))
    {
        box->setValue(loom::asFloat(value));
        return true;
    }

    if (QPlainTextEdit* box = qobject_cast<QPlainTextEdit*>(editor))
    {
        const QString text = QString::fromStdString(loom::asString(value));

        if (box->toPlainText() != text) box->setPlainText(text);
        return true;
    }

    if (QLineEdit* field = qobject_cast<QLineEdit*>(editor))
    {
        field->setText(QString::fromStdString(loom::asString(value)));
        return true;
    }

    if (QComboBox* box = qobject_cast<QComboBox*>(editor))
    {
        const int found = box->findData(QString::fromStdString(loom::asString(value)));

        // A name the list does not offer needs the whole editor built again.
        if (found < 0) return false;

        box->setCurrentIndex(found);
        return true;
    }

    return false;
}
