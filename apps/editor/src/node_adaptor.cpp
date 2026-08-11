#include "node_adaptor.h"

#include <QJsonDocument>

#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
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

    for (const loom::PinSpec& pin : type.pins(data.extraPins))
    {
        if (pin.direction == loom::PinDirection::Input) inputs.push_back(pin);
        else outputs.push_back(pin);
    }
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
