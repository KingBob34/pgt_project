#include "pin_editor.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>

#include "prose_editor.h"
#include <QSignalBlocker>
#include <QSpinBox>

#include "node_metrics.h"

#include "loom/qt/convert.h"
#include "loom/value/inspect.h"

using loom::qt::toQt;

namespace
{
    constexpr int    kIntLimit   = 1000000000;
    constexpr double kFloatLimit = 1e9;

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
        const QString text = toQt(loom::asString(value));
        const QString hint = toQt(pin.label);

        if (pin.textShape != loom::TextShape::Line)
        {
            QPlainTextEdit* box = new QPlainTextEdit(text);
            box->setPlaceholderText(hint);
            box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            QObject::connect(box, &QPlainTextEdit::textChanged, [box, changed]
            {
                changed(box->toPlainText().toStdString());
            });

            return { box, metrics::labelRows };
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
        const QString chosen = toQt(loom::asString(value));

        QComboBox* box = new QComboBox;

        // Nothing picked yet shows as an empty box rather than as an empty row
        // in the list: a blank is the state a new node starts in, not one of
        // the things there are to choose.
        box->setPlaceholderText(QString());

        // Every declared variable and every field nested in one, so a node can
        // reach into a group without a walk of its own.
        for (const std::string& path : loom::variablePaths(variables))
        {
            const QString name = toQt(path);
            const QString type = toQt(loom::declaredTypeAt(variables, path));

            box->addItem(name + "  (" + type + ")", name);
        }

        if (!chosen.isEmpty() && box->findData(chosen) < 0) box->addItem(chosen + " (missing)", chosen);

        box->setCurrentIndex(box->findData(chosen));

        QObject::connect(box, &QComboBox::currentIndexChanged, box, [box, changed](int)
        {
            changed(box->currentData().toString().toStdString());
        });

        return { box, 1 };
    }

    // A name out of a list, so a scene the story does not have cannot be typed
    // by accident. One that has since been deleted is still shown, so the
    // author can see which node is left pointing at nothing.
    PinEditor makeScene(const loom::Value& value, const PinChanged& changed,
                        const std::vector<std::string>& scenes)
    {
        const QString chosen = toQt(loom::asString(value));

        QComboBox* box = new QComboBox;
        box->setPlaceholderText(QString());

        for (const std::string& scene : scenes) box->addItem(toQt(scene), toQt(scene));

        if (!chosen.isEmpty() && box->findData(chosen) < 0)
        {
            box->addItem(chosen + " (missing)", chosen);
        }

        box->setCurrentIndex(box->findData(chosen));

        QObject::connect(box, &QComboBox::currentIndexChanged, box, [box, changed](int)
        {
            changed(box->currentData().toString().toStdString());
        });

        return { box, 1 };
    }

    PinEditor makeProse(const loom::Value& value, const PinChanged& changed, ProseSlots offer)
    {
        ProseEditor* box = new ProseEditor(std::move(offer));
        box->setPassage(value);

        QObject::connect(box, &ProseEditor::edited, box, [box, changed]
        {
            changed(box->passage());
        });

        return { box, metrics::proseRows };
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
                        const std::map<std::string, loom::VariableSpec>& variables,
                        const std::vector<std::string>& scenes,
                        ProseSlots offer)
{
    if (pin.type == loom::PinType::VariableName) return makeVariable(value, changed, variables);
    if (pin.type == loom::PinType::SceneName) return makeScene(value, changed, scenes);
    if (pin.type == loom::PinType::Prose) return makeProse(value, changed, std::move(offer));

    const auto factory = editableTypes().find(pin.type);

    if (factory == editableTypes().end()) return {};

    return factory->second(pin, value, changed);
}

void fitToNode(QWidget* editor, const loom::PinSpec& pin)
{
    if (editor == nullptr) return;

    if (pin.type == loom::PinType::VariableName || pin.type == loom::PinType::SceneName)
    {
        editor->setFixedWidth(metrics::variableWidth);
        return;
    }

    if (pin.type == loom::PinType::Prose)
    {
        editor->setFixedWidth(metrics::proseWidth);
        return;
    }

    if (pin.type == loom::PinType::String)
    {
        editor->setFixedWidth(pin.textShape == loom::TextShape::Label ? metrics::labelWidth
                                                                     : metrics::textWidth);

        return;
    }

    if (pin.type == loom::PinType::Int || pin.type == loom::PinType::Float)
    {
        editor->setFixedWidth(metrics::numberWidth);
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

    if (ProseEditor* box = qobject_cast<ProseEditor*>(editor))
    {
        box->setPassage(value);
        return true;
    }

    if (QPlainTextEdit* box = qobject_cast<QPlainTextEdit*>(editor))
    {
        const QString text = toQt(loom::asString(value));

        if (box->toPlainText() != text) box->setPlainText(text);
        return true;
    }

    if (QLineEdit* field = qobject_cast<QLineEdit*>(editor))
    {
        field->setText(toQt(loom::asString(value)));
        return true;
    }

    if (QComboBox* box = qobject_cast<QComboBox*>(editor))
    {
        const int found = box->findData(toQt(loom::asString(value)));

        // A name the list does not offer needs the whole editor built again.
        if (found < 0) return false;

        box->setCurrentIndex(found);
        return true;
    }

    return false;
}
