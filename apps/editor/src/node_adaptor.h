#ifndef LOOM_EDITOR_NODE_ADAPTOR_H
#define LOOM_EDITOR_NODE_ADAPTOR_H
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <QPointer>
#include <QSize>

#include <QtNodes/AbstractGraphModel>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "prose_editor.h"

#include "loom/graph/catalog.h"
#include "loom/graph/graph.h"

// How a node reaches the graph it sits in. The model is built after the
// registry that makes the nodes, so it is fetched when wanted rather than held.
using GraphOwner = std::function<QtNodes::AbstractGraphModel*()>;

// The editor's only NodeDelegateModel. Forwards every question QtNodes asks
// about a node to the NodeType it shares with the engine.
class NodeAdaptor : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    // The declared variables outlive every node on the canvas.
    NodeAdaptor(const loom::NodeType& nodeType,
                const std::map<std::string, loom::VariableSpec>& variableSpecs,
                GraphOwner graphOwner);

    QString name()    const override;
    QString caption() const override;

    unsigned int          nPorts(QtNodes::PortType portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex index) const override;
    QString               portCaption(QtNodes::PortType portType, QtNodes::PortIndex index) const override;
    bool                  portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override { return true; }

    QtNodes::ConnectionPolicy portConnectionPolicy(QtNodes::PortType portType,
                                                   QtNodes::PortIndex index) const override;

    QWidget* embeddedWidget() override;

    // Unused: values travel through the engine, not through QtNodes.
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return nullptr; }

    QJsonObject save() const override;
    void        load(const QJsonObject& document) override;

    const loom::NodeType&     nodeType() const { return type; }

    // A frame is drawn, sized and moved by the canvas rather than by its rows.
    bool isFrame() const { return type.isFrame(); }

    // Whether the author drags this node to a size of their own.
    bool isResizable() const { return type.isResizable(); }

    // How big it has been dragged to, and the smallest it may be -- which is
    // also the size a new one is made at.
    QSize boxSize() const;
    QSize leastSize() const;
    void  setBoxSize(QSize size);
    const loom::NodeInstance& instance() const { return data; }

    const std::map<std::string, loom::VariableSpec>& variableSpecs() const { return variables; }

    // What a pin carries once the variable it follows has been chosen.
    std::string resolvedType(const loom::PinSpec& pin) const;

    // Rebuilds the editors, for when the declared variables have changed. The
    // update is what makes the ports ask for their type again.
    void refreshEditors()
    {
        rebuildEditors();

        Q_EMIT requestNodeUpdate();
    }

    // Everything except id and position, which stay with QtNodes.
    void setInstance(const loom::NodeInstance& instance);

    // Pin identity is the name; the index is QtNodes' own bookkeeping.
    std::string        pinName  (QtNodes::PortType portType, QtNodes::PortIndex index) const;
    QtNodes::PortIndex portIndex(QtNodes::PortType portType, const std::string& pin) const;

    // The author's control over a variadic node's pin count.
    void setExtraPins(int count);

    // Where a pin's row sits inside the body widget, in pixels from its top.
    int rowTop(QtNodes::PortType portType, QtNodes::PortIndex index) const;
    int rowHeight(QtNodes::PortType portType, QtNodes::PortIndex index) const;

    // Writes a pin's value from outside the node and redraws its editor.
    void setPinValue(const std::string& pin, loom::Value value);

    bool isWired(const std::string& pin) const;

    // The Any inputs a passage on this node may quote, numbered in the order
    // they are declared. Each is named after whatever feeds it, so a chip in
    // the text reads as the variable behind it rather than as a pin number.
    std::vector<ProseSlot> valueSlots() const;

    // What to write on a chip standing for one of this node's outputs.
    QString slotNameFor(QtNodes::PortIndex port) const;

    // The pins the author may type into. A value node holds its constant on its
    // output pin; every other node, flow or not, is filled in on its inputs.
    const std::vector<loom::PinSpec>& editablePins() const
    {
        return holdsConstant() ? outputs : inputs;
    }

Q_SIGNALS:
    // Raised by the editor on the node itself, not by setPinValue.
    void pinValueTyped(const QString& pin);

public Q_SLOTS:
    void inputConnectionCreated(const QtNodes::ConnectionId& connection) override;
    void inputConnectionDeleted(const QtNodes::ConnectionId& connection) override;

private:
    const loom::PinSpec* pinAt(QtNodes::PortType portType, QtNodes::PortIndex index) const;

    // The row a port sits on. The two are numbered apart because a pin that
    // takes no wire still has a row of its own.
    std::size_t rowOfPort(QtNodes::PortType portType, QtNodes::PortIndex index) const;

    void refreshPins();
    void rebuildEditors();
    QWidget* buildPinButtons();
    void setWired(const std::string& pin, bool on);

    // Puts the passage box at the size the node has been dragged to, and
    // brings the card that carries it in line with its rows.
    void sizeBox(QSize box);
    void fitBody();

    // A node that asks for nothing and answers with a constant of its own, so
    // the box the author types in belongs on its output. Having no flow pins
    // is not enough to tell: every pure node is off the flow, and most of them
    // do have inputs. Both the editors and the row heights hang off this one
    // answer.
    bool holdsConstant() const { return inputs.empty() && !outputs.empty(); }

    // The side whose pins carry the editors.
    bool edited(QtNodes::PortType portType) const;
    loom::Value pinValue(const loom::PinSpec& pin) const;

    const loom::NodeType& type;
    loom::NodeInstance    data;

    const std::map<std::string, loom::VariableSpec>& variables;

    std::vector<loom::PinSpec> inputs;
    std::vector<loom::PinSpec> outputs;

    // Which of the inputs carry a port, in port order.
    std::vector<std::size_t> inputPorts;

    std::vector<int>                rowHeights;
    QPointer<QWidget>               body;
    std::map<std::string, QWidget*> editors;
    std::set<std::string>           wired;

    // The wire reaching each connected input, so the node can ask what is on
    // the far end of it.
    std::map<std::string, QtNodes::ConnectionId> feeds;

    GraphOwner owner;
};

// One registry entry per node type, grouped by the category the node declares.
std::shared_ptr<QtNodes::NodeDelegateModelRegistry> makeRegistry(
    const loom::NodeCatalog& catalog, const std::map<std::string, loom::VariableSpec>& variableSpecs,
    GraphOwner graphOwner);

// The adaptor behind a node id. Null when the model is not one of ours, which
// the geometry and the painters all have to ask about.
NodeAdaptor* adaptorFor(QtNodes::AbstractGraphModel& model, QtNodes::NodeId node);

#endif //LOOM_EDITOR_NODE_ADAPTOR_H
