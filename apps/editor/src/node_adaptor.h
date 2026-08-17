#ifndef LOOM_EDITOR_NODE_ADAPTOR_H
#define LOOM_EDITOR_NODE_ADAPTOR_H
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <QPointer>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "loom/graph/catalog.h"
#include "loom/graph/graph.h"

// The editor's only NodeDelegateModel. Forwards every question QtNodes asks
// about a node to the NodeType it shares with the engine.
class NodeAdaptor : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    // The declared variables outlive every node on the canvas.
    NodeAdaptor(const loom::NodeType& nodeType,
                const std::map<std::string, loom::VariableSpec>& variableSpecs);

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

    // The pins the author may type into. A value node holds its constant on its
    // output pin; every other node, flow or not, is filled in on its inputs.
    const std::vector<loom::PinSpec>& editablePins() const
    {
        return constant && !outputs.empty() ? outputs : inputs;
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

    // The side whose pins carry the editors.
    bool edited(QtNodes::PortType portType) const;
    loom::Value pinValue(const loom::PinSpec& pin) const;

    const loom::NodeType& type;
    loom::NodeInstance    data;

    const std::map<std::string, loom::VariableSpec>& variables;

    std::vector<loom::PinSpec> inputs;
    std::vector<loom::PinSpec> outputs;
    bool                       constant = false;

    // Which of the inputs carry a port, in port order.
    std::vector<std::size_t> inputPorts;

    std::vector<int>                rowHeights;
    QPointer<QWidget>               body;
    std::map<std::string, QWidget*> editors;
    std::set<std::string>           wired;
};

// One registry entry per node type, grouped by the category the node declares.
std::shared_ptr<QtNodes::NodeDelegateModelRegistry> makeRegistry(
    const loom::NodeCatalog& catalog, const std::map<std::string, loom::VariableSpec>& variableSpecs);

#endif //LOOM_EDITOR_NODE_ADAPTOR_H
