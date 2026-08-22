#include "node_adaptor.h"

#include <algorithm>

#include <QJsonDocument>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>

#include <QtNodes/DataFlowGraphModel>

#include "node_metrics.h"
#include "node_palette.h"
#include "pin_editor.h"

#include "loom/value/inspect.h"
#include "loom/value/parse.h"

namespace
{
    const char* const kPinButtonStyle =
        "QPushButton {"
        "  color: #c8c8c8;"
        "  background: rgba(255, 255, 255, 20);"
        "  border: 1px solid rgba(255, 255, 255, 30);"
        "  border-radius: 3px;"
        "  padding: 0;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 45); }"
        "QPushButton:disabled { color: #6a6a6a; border-color: rgba(255, 255, 255, 15); }";

    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }

    // The pins on one side of a node as the canvas shows them. Kept to the
    // same rule refreshPins uses, because the two lists are compared with
    // each other to work out which ports came and went.
    std::vector<loom::PinSpec> onSide(const std::vector<loom::PinSpec>& pins,
                                      loom::PinDirection direction)
    {
        std::vector<loom::PinSpec> out;

        for (const loom::PinSpec& pin : pins)
        {
            if (pin.direction == direction && !pin.hidden) out.push_back(pin);
        }

        return out;
    }

    // The pins on a side that carry a connector. A pin the author fills in on
    // the node itself has a row but no port, so it is no part of the numbering
    // the canvas uses.
    std::vector<loom::PinSpec> ported(const std::vector<loom::PinSpec>& pins)
    {
        std::vector<loom::PinSpec> out;

        for (const loom::PinSpec& pin : pins)
        {
            if (pin.type != loom::PinType::VariableName && pin.type != loom::PinType::Prose)
            {
                out.push_back(pin);
            }
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

NodeAdaptor::NodeAdaptor(const loom::NodeType& nodeType,
                         const std::map<std::string, loom::VariableSpec>& variableSpecs,
                         GraphOwner graphOwner)
    : type(nodeType), variables(variableSpecs), owner(std::move(graphOwner))
{
    data.type = nodeType.name();
    data.extraPins = nodeType.minExtraPins();

    refreshPins();
}

void NodeAdaptor::refreshPins()
{
    inputs.clear();
    outputs.clear();
    inputPorts.clear();

    for (const loom::PinSpec& pin : type.pins(data.extraPins))
    {
        // The size the author dragged the node to travels in the file but is
        // no part of what the node asks for.
        if (pin.hidden) continue;

        if (pin.direction != loom::PinDirection::Input)
        {
            outputs.push_back(pin);
            continue;
        }

        // A variable is chosen from a list and a passage is written in place,
        // so neither row is given a connector.
        if (pin.type != loom::PinType::VariableName && pin.type != loom::PinType::Prose)
        {
            inputPorts.push_back(inputs.size());
        }

        inputs.push_back(pin);
    }

    // A frame has no rows at all. Its title and its size still live on pins,
    // because that is what gets written to the file, but they are set by
    // dragging its corner and by typing on the frame itself, so it offers
    // neither a connector nor an editor.
    if (isFrame())
    {
        inputs.clear();
        inputPorts.clear();
        outputs.clear();
    }

    rebuildEditors();
}

QString NodeAdaptor::name() const
{
    return toQt(type.name());
}

QString NodeAdaptor::caption() const
{
    // A frame carries the author's own title where a node carries its type.
    if (isFrame())
    {
        const auto titled = data.pinValues.find("text");

        if (titled != data.pinValues.end()) return toQt(loom::asString(titled->second));
    }

    return toQt(type.displayName());
}

QSize NodeAdaptor::leastSize() const
{
    QSize least;

    // The size a node declares it starts at is the size it may not go below.
    for (const loom::PinSpec& pin : type.pins(data.extraPins))
    {
        if (pin.name == "width")  least.setWidth(int(loom::asInt(pin.defaultValue)));
        if (pin.name == "height") least.setHeight(int(loom::asInt(pin.defaultValue)));
    }

    return least;
}

QSize NodeAdaptor::boxSize() const
{
    const QSize least = leastSize();

    const auto across = data.pinValues.find("width");
    const auto down   = data.pinValues.find("height");

    return QSize(across == data.pinValues.end() ? least.width() : int(loom::asInt(across->second)),
                 down   == data.pinValues.end() ? least.height() : int(loom::asInt(down->second)));
}

void NodeAdaptor::setBoxSize(QSize size)
{
    const QSize least = leastSize();
    const QSize kept(std::max(size.width(), least.width()),
                     std::max(size.height(), least.height()));

    data.pinValues["width"]  = loom::Value(kept.width());
    data.pinValues["height"] = loom::Value(kept.height());

    // A frame is drawn at this size and carries nothing. Every other node is
    // as big as what stands on it, so the box grows and the card follows.
    if (!isFrame()) sizeBox(kept);

    Q_EMIT requestNodeUpdate();
}

// The passage box is the one editor whose size the author sets, so it is the
// one that is not measured in rows.
void NodeAdaptor::sizeBox(QSize box)
{
    std::size_t row = 0;

    for (const loom::PinSpec& pin : editablePins())
    {
        if (pin.type == loom::PinType::Prose)
        {
            const auto found = editors.find(pin.name);

            if (found != editors.end() && found->second != nullptr)
            {
                found->second->setFixedSize(box);

                // The row it stands on is that tall now, which is what the
                // card is measured from and where the ports below it sit.
                if (row < rowHeights.size()) rowHeights[row] = box.height();
            }
        }

        ++row;
    }

    fitBody();
}

// The rows, the gaps between them, and the buttons under them.
//
// QtNodes measures a node from the width and height its widget is standing at,
// not from the size it would like to be, so the body is pinned to what its
// rows need rather than left for the layout to settle on its own.
void NodeAdaptor::fitBody()
{
    if (body.isNull()) return;

    QLayout* column = body->layout();
    if (column == nullptr) return;

    int total = 0;
    for (int height : rowHeights) total += height + metrics::rowGap;

    if (type.maxExtraPins() > type.minExtraPins()) total += metrics::rowHeight();

    // No shorter than the other side's ports, or the node centres the widget
    // and the rows stop lining up.
    const std::size_t others = holdsConstant() ? inputs.size() : outputs.size();

    // Asked of the rows themselves. A row that was given a fixed width answers
    // with it whether or not the layout has run yet; the body as a whole does
    // not, which is why it is not asked.
    int widest = metrics::minimumWidth;

    for (int at = 0; at < column->count(); ++at)
    {
        QWidget* row = column->itemAt(at)->widget();
        if (row == nullptr) continue;

        widest = std::max(widest, std::max(row->minimumWidth(), row->sizeHint().width()));
    }

    body->setFixedSize(widest, std::max(total, static_cast<int>(others) * metrics::rowHeight()));
}

unsigned int NodeAdaptor::nPorts(QtNodes::PortType portType) const
{
    const std::size_t count = portType == QtNodes::PortType::In ? inputPorts.size() : outputs.size();

    return static_cast<unsigned int>(count);
}

const loom::PinSpec* NodeAdaptor::pinAt(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    if (index < 0) return nullptr;

    const std::size_t at = static_cast<std::size_t>(index);

    if (portType == QtNodes::PortType::Out)
    {
        return at < outputs.size() ? &outputs[at] : nullptr;
    }

    return at < inputPorts.size() ? &inputs[inputPorts[at]] : nullptr;
}

std::size_t NodeAdaptor::rowOfPort(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const std::size_t at = index < 0 ? 0 : static_cast<std::size_t>(index);

    if (portType != QtNodes::PortType::In || at >= inputPorts.size()) return at;

    return inputPorts[at];
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

    const std::vector<loom::PinSpec> asked = onSide(pins, loom::PinDirection::Input);

    // Values are erased by pin and ports are announced by port. The two are
    // different counts on a node whose rows are not all wired to: the passage
    // on Show Text is a row with no connector under it.
    unsigned int firstPin = 0;
    unsigned int spanPin  = 0;

    changedRange(inputs, asked, firstPin, spanPin);
    changedRange(ported(inputs), ported(asked), first, span);

    if (shrinking)
    {
        // A pin the author took away takes its value with it.
        for (unsigned int index = firstPin; index < firstPin + spanPin; ++index)
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
    for (unsigned int index = 0; index < nPorts(portType); ++index)
    {
        const loom::PinSpec* spec = pinAt(portType, static_cast<QtNodes::PortIndex>(index));

        if (spec != nullptr && spec->name == pin) return static_cast<QtNodes::PortIndex>(index);
    }

    return QtNodes::InvalidPortIndex;
}

std::string NodeAdaptor::resolvedType(const loom::PinSpec& pin) const
{
    if (pin.typeFollows.empty()) return pin.type;

    const auto chosen = data.pinValues.find(pin.typeFollows);
    if (chosen == data.pinValues.end()) return loom::PinType::Unset;

    const std::string declared =
        loom::declaredTypeAt(variables, loom::asString(chosen->second));

    return declared.empty() ? loom::PinType::Unset : loom::pinTypeOfVariable(declared);
}

QtNodes::NodeDataType NodeAdaptor::dataType(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const loom::PinSpec* pin = pinAt(portType, index);
    if (pin == nullptr) return {};

    const std::string type = resolvedType(*pin);

    return { toQt(type), toQt(loom::pinTypeLabel(type)) };
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
    if (isFrame()) return nullptr;

    if (body.isNull())
    {
        body = new QWidget;
        body->setAttribute(Qt::WA_TranslucentBackground);

        QVBoxLayout* column = new QVBoxLayout(body);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(metrics::rowGap);

        rebuildEditors();
    }

    return body;
}

void NodeAdaptor::rebuildEditors()
{
    if (body.isNull()) return;

    editors.clear();
    rowHeights.clear();

    QVBoxLayout* column = static_cast<QVBoxLayout*>(body->layout());

    while (QLayoutItem* item = column->takeAt(0))
    {
        // Deferred, because one of these may be the button that asked for it.
        // The parent stays put: a widget without one is a window of its own.
        if (QWidget* widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }

        delete item;
    }

    for (const loom::PinSpec& pin : editablePins())
    {
        // A chosen variable decides another pin's type, so the ports repaint.
        const bool governs = pin.type == loom::PinType::VariableName;

        const PinEditor made = makePinEditor(pin, pinValue(pin),
                                             [this, name = pin.name, governs](loom::Value value)
                                             {
                                                 data.pinValues[name] = std::move(value);

                                                 Q_EMIT pinValueTyped(QString::fromStdString(name));

                                                 if (governs) Q_EMIT requestNodeUpdate();
                                             },
                                             variables,
                                             [this] { return valueSlots(); });

        QWidget* editor = made.widget;

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

        fitToNode(made.widget, pin);

        const bool dragged = pin.type == loom::PinType::Prose && type.isResizable();

        if (dragged) editor->setFixedWidth(boxSize().width());

        const int height = dragged ? boxSize().height()
                                   : metrics::rowHeight() * std::max(1, made.rows);

        editor->setFixedHeight(height);
        column->addWidget(editor);

        rowHeights.push_back(height);
    }

    if (type.maxExtraPins() > type.minExtraPins()) column->addWidget(buildPinButtons());

    column->addStretch();

    fitBody();
}

void NodeAdaptor::setPinValue(const std::string& pin, loom::Value value)
{
    data.pinValues[pin] = std::move(value);

    // The row goes first, so that a keystroke does not rebuild the whole body.
    const auto shown = editors.find(pin);

    if (shown != editors.end() && showInEditor(shown->second, data.pinValues[pin])) return;

    rebuildEditors();

    Q_EMIT requestNodeUpdate();
}

bool NodeAdaptor::isWired(const std::string& pin) const
{
    return wired.count(pin) != 0;
}

bool NodeAdaptor::edited(QtNodes::PortType portType) const
{
    const QtNodes::PortType side = holdsConstant() ? QtNodes::PortType::Out
                                                   : QtNodes::PortType::In;

    return portType == side;
}

int NodeAdaptor::rowHeight(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const std::size_t row = rowOfPort(portType, index);

    if (!edited(portType) || row >= rowHeights.size()) return metrics::rowHeight();

    return rowHeights[row];
}

int NodeAdaptor::rowTop(QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    const std::size_t row = rowOfPort(portType, index);

    if (!edited(portType)) return static_cast<int>(row) * metrics::rowHeight();

    int top = 0;

    for (std::size_t at = 0; at < row && at < rowHeights.size(); ++at)
    {
        top += rowHeights[at] + metrics::rowGap;
    }

    return top;
}

QWidget* NodeAdaptor::buildPinButtons()
{
    QWidget* row = new QWidget;
    row->setFixedHeight(metrics::rowHeight());

    QHBoxLayout* line = new QHBoxLayout(row);
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(4);

    QPushButton* fewer = new QPushButton("-");
    QPushButton* more  = new QPushButton("+");

    // Flat and quiet, so a control that shapes the node does not read as one
    // of the values on it.
    for (QPushButton* button : { fewer, more })
    {
        button->setFixedWidth(metrics::pinButtonWidth);
        button->setStyleSheet(kPinButtonStyle);
    }

    fewer->setEnabled(data.extraPins > type.minExtraPins());
    more->setEnabled(data.extraPins < type.maxExtraPins());

    connect(fewer, &QPushButton::clicked, this, [this] { setExtraPins(data.extraPins - 1); });
    connect(more,  &QPushButton::clicked, this, [this] { setExtraPins(data.extraPins + 1); });

    // By the left edge, where the rows they add and take away begin. On a wide
    // node they would otherwise sit a long way from what they shape.
    line->addWidget(fewer);
    line->addWidget(more);
    line->addStretch();

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
    const std::string pin = pinName(QtNodes::PortType::In, connection.inPortIndex);

    feeds[pin] = connection;

    setWired(pin, true);
}

void NodeAdaptor::inputConnectionDeleted(const QtNodes::ConnectionId& connection)
{
    const std::string pin = pinName(QtNodes::PortType::In, connection.inPortIndex);

    feeds.erase(pin);

    setWired(pin, false);
}

std::vector<ProseSlot> NodeAdaptor::valueSlots() const
{
    std::vector<ProseSlot> offered;

    QtNodes::AbstractGraphModel* graph = owner ? owner() : nullptr;

    int at = 0;

    for (const loom::PinSpec& pin : inputs)
    {
        if (pin.type != loom::PinType::Any) continue;

        // The slot number is the pin's place among the value pins, whether or
        // not anything reaches it, because that is what the passage stores.
        const int slotIndex = at++;

        // An empty pin has nothing to quote. Offering it would put a chip in
        // the passage that reads as nothing at all.
        const auto wire = feeds.find(pin.name);

        if (graph == nullptr || wire == feeds.end()) continue;

        const NodeAdaptor* source = adaptorFor(*graph, wire->second.outNodeId);
        if (source == nullptr) continue;

        ProseSlot slot;
        slot.index = slotIndex;

        // Named after whatever is on the far end of the wire, so the chip in
        // the passage reads the way the author thinks of that value.
        slot.label = source->slotNameFor(wire->second.outPortIndex);
        slot.colour = palette::pin(
            source->dataType(QtNodes::PortType::Out, wire->second.outPortIndex).id.toStdString());

        offered.push_back(slot);
    }

    return offered;
}

QString NodeAdaptor::slotNameFor(QtNodes::PortIndex port) const
{
    // A node that reads a variable is called after the variable, which is what
    // the author picked it for. Anything else answers with its own name.
    const auto chosen = data.pinValues.find("name");

    if (chosen != data.pinValues.end())
    {
        const QString named = toQt(loom::asString(chosen->second));

        if (!named.isEmpty()) return named;
    }

    const loom::PinSpec* pin = pinAt(QtNodes::PortType::Out, port);

    if (pin != nullptr && !pin->label.empty()) return caption() + ": " + toQt(pin->label);

    return caption();
}

NodeAdaptor* adaptorFor(QtNodes::AbstractGraphModel& model, QtNodes::NodeId node)
{
    QtNodes::DataFlowGraphModel* flow = dynamic_cast<QtNodes::DataFlowGraphModel*>(&model);

    return flow == nullptr ? nullptr : flow->delegateModel<NodeAdaptor>(node);
}

std::shared_ptr<QtNodes::NodeDelegateModelRegistry> makeRegistry(
    const loom::NodeCatalog& catalog, const std::map<std::string, loom::VariableSpec>& variableSpecs,
    GraphOwner graphOwner)
{
    auto registry = std::make_shared<QtNodes::NodeDelegateModelRegistry>();

    for (const loom::NodeType* type : catalog.all())
    {
        registry->registerModel<NodeAdaptor>(
            [type, &variableSpecs, graphOwner]
            {
                return std::make_unique<NodeAdaptor>(*type, variableSpecs, graphOwner);
            },
            toQt(type->category()));
    }

    return registry;
}
