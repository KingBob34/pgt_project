#include "node_adaptor.h"

#include <algorithm>

#include <QJsonDocument>
#include <QVBoxLayout>
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>

#include "node_geometry.h"
#include "pin_editor.h"

#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }
    std::vector<loom::PinSpec> onSide(const std::vector<loom::PinSpec>& pins,
                                  loom::PinDirection direction)
    {
        std::vector<loom::PinSpec> out;

        for (const loom::PinSpec& pin : pins)
        {
            if (pin.direction == direction) out.push_back(pin);
        }

        return out;
    }

    // Variadic pins are one unbroken run, so what changed between two pin lists
    // is whatever is left after matching names in from both ends.
    void changedRange(const std::vector<loom::PinSpec>& before,
                      const std::vector<loom::PinSpec>& after,
                      unsigned int& first, unsigned int& span)
    {
        const std::size_t shorter = std::min(before.size(), after.size());

        std::size_t head = 0;
        while (head < shorter && before[head].name == after[head].name) ++head;

        std::size_t tail = 0;
        while (tail < shorter - head &&
               before[before.size() - 1 - tail].name == after[after.size() - 1 - tail].name)
        {
            ++tail;
        }

        first = static_cast<unsigned int>(head);
        span  = static_cast<unsigned int>(std::max(before.size(), after.size()) - head - tail);
    }
}

NodeAdaptor::NodeAdaptor(const loom::NodeType& nodeType)
    : type(nodeType)
{
    data.type = nodeType.name();
    data.extraPins = nodeType.minExtraPins();

    refreshPins();
}

void NodeAdaptor::refreshPins()
{
    inputs.clear();
    outputs.clear();
    constant = true;

    for (const loom::PinSpec& pin : type.pins(data.extraPins))
    {
        if (pin.type == loom::PinType::Flow) constant = false;

        if (pin.direction == loom::PinDirection::Input) inputs.push_back(pin);
        else outputs.push_back(pin);
    }

    rebuildEditors();
}

QString NodeAdaptor::name() const
{
    return toQt(type.name());
}

QString NodeAdaptor::caption() const
{
    return toQt(type.displayName());
}

unsigned int NodeAdaptor::nPorts(QtNodes::PortType portType) const
{
    const std::vector<loom::PinSpec>& pins = portType == QtNodes::PortType::In ? inputs : outputs;

    return static_cast<unsigned int>(pins.size());
}

const loom::PinSpec* NodeAdaptor::pinAt(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const std::vector<loom::PinSpec>& pins = portType == QtNodes::PortType::In ? inputs : outputs;

    if (index < 0 || static_cast<std::size_t>(index) >= pins.size()) return nullptr;

    return &pins[static_cast<std::size_t>(index)];
}

void NodeAdaptor::setInstance(const loom::NodeInstance& instance)
{
    data.extraPins = instance.extraPins;
    data.pinValues = instance.pinValues;

    refreshPins();

    Q_EMIT requestNodeUpdate();
}

void NodeAdaptor::setExtraPins(int count)
{
    const int wanted = std::max(type.minExtraPins(), std::min(count, type.maxExtraPins()));

    if (wanted == data.extraPins) return;

    const std::vector<loom::PinSpec> pins = type.pins(wanted);
    const bool shrinking = wanted < data.extraPins;

    unsigned int first = 0;
    unsigned int span  = 0;

    // Outputs only come and go at the end, so all they need is for a removed
    // pin's wire to be let go of while the pin is still there.
    if (shrinking)
    {
        changedRange(outputs, onSide(pins, loom::PinDirection::Output), first, span);

        Q_EMIT portsAboutToBeDeleted(QtNodes::PortType::Out, first, first + span - 1);
        Q_EMIT portsDeleted();
    }

    changedRange(inputs, onSide(pins, loom::PinDirection::Input), first, span);

    if (shrinking)
    {
        // A pin the author took away takes its value with it.
        for (unsigned int index = first; index < first + span; ++index)
        {
            data.pinValues.erase(inputs[index].name);
        }

        Q_EMIT portsAboutToBeDeleted(QtNodes::PortType::In, first, first + span - 1);
    }
    else
    {
        Q_EMIT portsAboutToBeInserted(QtNodes::PortType::In, first, first + span - 1);
    }

    data.extraPins = wanted;
    refreshPins();

    if (shrinking) Q_EMIT portsDeleted();
    else           Q_EMIT portsInserted();

    Q_EMIT requestNodeUpdate();
}

std::string NodeAdaptor::pinName(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const loom::PinSpec* pin = pinAt(portType, index);

    return pin == nullptr ? std::string() : pin->name;
}

QtNodes::PortIndex NodeAdaptor::portIndex(QtNodes::PortType portType, const std::string& pin) const
{
    const std::vector<loom::PinSpec>& pins = portType == QtNodes::PortType::In ? inputs : outputs;

    for (std::size_t index = 0; index < pins.size(); ++index)
    {
        if (pins[index].name == pin) return static_cast<QtNodes::PortIndex>(index);
    }

    return QtNodes::InvalidPortIndex;
}

QtNodes::NodeDataType NodeAdaptor::dataType(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const loom::PinSpec* pin = pinAt(portType, index);
    if (pin == nullptr) return {};

    return { toQt(pin->type), toQt(loom::pinTypeLabel(pin->type)) };
}

QString NodeAdaptor::portCaption(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const loom::PinSpec* pin = pinAt(portType, index);

    return pin == nullptr ? QString() : toQt(pin->label);
}

QtNodes::ConnectionPolicy NodeAdaptor::portConnectionPolicy(QtNodes::PortType portType,
                                                            QtNodes::PortIndex index) const
{
    const loom::PinSpec* pin = pinAt(portType, index);
    if (pin == nullptr) return QtNodes::ConnectionPolicy::One;

    const bool flow = pin->type == loom::PinType::Flow;
    const bool output = portType == QtNodes::PortType::Out;

    // The side that has to resolve to one thing takes one wire.
    return flow == output ? QtNodes::ConnectionPolicy::One : QtNodes::ConnectionPolicy::Many;
}

QJsonObject NodeAdaptor::save() const
{
    QJsonObject out = QtNodes::NodeDelegateModel::save();

    if (data.extraPins != 0) out["extraPins"] = data.extraPins;

    if (!data.pinValues.empty())
    {
        loom::Value values = loom::Value::object();
        for (const auto& entry : data.pinValues) values[entry.first] = entry.second;

        out["pinValues"] = QJsonDocument::fromJson(
            QByteArray::fromStdString(loom::writeJson(values))).object();
    }

    return out;
}

void NodeAdaptor::load(const QJsonObject& document)
{
    QtNodes::NodeDelegateModel::load(document);

    data.extraPins = document["extraPins"].toInt(type.minExtraPins());
    data.pinValues.clear();

    const QJsonObject values = document["pinValues"].toObject();
    if (!values.isEmpty())
    {
        const QByteArray text = QJsonDocument(values).toJson(QJsonDocument::Compact);

        loom::Value parsed;
        std::string error;
        if (loom::parseJson(text.toStdString(), parsed, error))
        {
            for (const std::string& key : loom::objectKeys(parsed))
            {
                if (const loom::Value* stored = loom::objectGet(parsed, key)) data.pinValues[key] = *stored;
            }
        }
    }

    refreshPins();
    Q_EMIT requestNodeUpdate();
}

QWidget* NodeAdaptor::embeddedWidget()
{
    if (body.isNull())
    {
        body = new QWidget;
        body->setAttribute(Qt::WA_TranslucentBackground);

        QVBoxLayout* column = new QVBoxLayout(body);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(0);

        rebuildEditors();
    }

    return body;
}

void NodeAdaptor::rebuildEditors()
{
    if (body.isNull()) return;

    editors.clear();

    QVBoxLayout* column = static_cast<QVBoxLayout*>(body->layout());

    while (QLayoutItem* item = column->takeAt(0))
    {
        // Deferred, because one of these may be the button that asked for it.
        if (QWidget* widget = item->widget())
        {
            widget->setParent(nullptr);
            widget->deleteLater();
        }

        delete item;
    }

    // A value node has no flow pins, so its constant is the output pin itself.
    const std::vector<loom::PinSpec>& pins = constant ? outputs : inputs;

    std::size_t rows = 0;

    for (const loom::PinSpec& pin : pins)
    {
        QWidget* editor = makePinEditor(pin, pinValue(pin),
                                        [this, name = pin.name](loom::Value value)
                                        {
                                            data.pinValues[name] = std::move(value);
                                        });

        if (editor == nullptr)
        {
            // A blank row, so the rows below still land on their own port.
            editor = new QWidget;
        }
        else
        {
            editor->setEnabled(wired.count(pin.name) == 0);
            editors[pin.name] = editor;
        }

        editor->setFixedHeight(portRowHeight());
        column->addWidget(editor);

        ++rows;
    }

    if (type.maxExtraPins() > type.minExtraPins())
    {
        column->addWidget(buildPinButtons());
        ++rows;
    }

    column->addStretch();

    // No shorter than the ports, or the node centres the widget and the rows
    // stop lining up.
    rows = std::max(rows, std::max(inputs.size(), outputs.size()));

    body->setFixedHeight(static_cast<int>(rows) * portRowHeight());
}

QWidget* NodeAdaptor::buildPinButtons()
{
    QWidget* row = new QWidget;
    row->setFixedHeight(portRowHeight());

    QHBoxLayout* line = new QHBoxLayout(row);
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(4);

    QPushButton* fewer = new QPushButton("-");
    QPushButton* more  = new QPushButton("+");

    fewer->setFixedWidth(24);
    more->setFixedWidth(24);

    fewer->setEnabled(data.extraPins > type.minExtraPins());
    more->setEnabled(data.extraPins < type.maxExtraPins());

    connect(fewer, &QPushButton::clicked, this, [this] { setExtraPins(data.extraPins - 1); });
    connect(more,  &QPushButton::clicked, this, [this] { setExtraPins(data.extraPins + 1); });

    line->addStretch();
    line->addWidget(fewer);
    line->addWidget(more);

    return row;
}

loom::Value NodeAdaptor::pinValue(const loom::PinSpec& pin) const
{
    const auto stored = data.pinValues.find(pin.name);

    return stored == data.pinValues.end() ? pin.defaultValue : stored->second;
}

void NodeAdaptor::setWired(const std::string& pin, bool on)
{
    if (on) wired.insert(pin);
    else    wired.erase(pin);

    const auto editor = editors.find(pin);
    if (editor != editors.end()) editor->second->setEnabled(!on);
}

void NodeAdaptor::inputConnectionCreated(const QtNodes::ConnectionId& connection)
{
    setWired(pinName(QtNodes::PortType::In, connection.inPortIndex), true);
}

void NodeAdaptor::inputConnectionDeleted(const QtNodes::ConnectionId& connection)
{
    setWired(pinName(QtNodes::PortType::In, connection.inPortIndex), false);
}

std::shared_ptr<QtNodes::NodeDelegateModelRegistry> makeRegistry(const loom::NodeCatalog& catalog)
{
    auto registry = std::make_shared<QtNodes::NodeDelegateModelRegistry>();

    for (const loom::NodeType* type : catalog.all())
    {
        registry->registerModel<NodeAdaptor>([type] { return std::make_unique<NodeAdaptor>(*type); },
                                             toQt(type->category()));
    }

    return registry;
}
