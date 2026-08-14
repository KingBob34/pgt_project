#include "details_panel.h"

#include <QFormLayout>
#include <QLabel>
#include <QLayoutItem>

#include "node_adaptor.h"
#include "pin_editor.h"

namespace
{
    constexpr int kParagraphHeight = 220;
}

DetailsPanel::DetailsPanel(QWidget* parent)
    : QWidget(parent)
{
    form = new QFormLayout(this);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
}

void DetailsPanel::clearForm()
{
    fields.clear();

    while (QLayoutItem* item = form->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }

        delete item;
    }
}

void DetailsPanel::setNode(NodeAdaptor* adaptor)
{
    clearForm();

    shown = adaptor;

    if (shown.isNull())
    {
        form->addRow(new QLabel("Nothing selected"));
        return;
    }

    connect(shown, &NodeAdaptor::pinValueTyped, this, &DetailsPanel::refresh);

    form->addRow(new QLabel("<b>" + shown->caption() + "</b>"));

    const loom::NodeInstance& node = shown->instance();

    for (const loom::PinSpec& pin : shown->editablePins())
    {
        const auto stored = node.pinValues.find(pin.name);
        const loom::Value& value = stored == node.pinValues.end() ? pin.defaultValue : stored->second;

        const PinEditor made = makePinEditor(pin, value,
                                             [this, name = pin.name](loom::Value written)
                                             {
                                                 if (shown.isNull()) return;

                                                 shown->setPinValue(name, std::move(written));
                                             });

        if (made.widget == nullptr) continue;

        if (pin.longText) made.widget->setMinimumHeight(kParagraphHeight);

        made.widget->setEnabled(!shown->isWired(pin.name));

        fields[pin.name] = made.widget;

        form->addRow(QString::fromStdString(pin.label), made.widget);
    }
}

void DetailsPanel::refresh(const QString& pin)
{
    if (shown.isNull()) return;

    const auto field = fields.find(pin.toStdString());

    if (field == fields.end()) return;

    const loom::NodeInstance& node = shown->instance();
    const auto stored = node.pinValues.find(pin.toStdString());

    if (stored != node.pinValues.end()) showInEditor(field->second, stored->second);
}
