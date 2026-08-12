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

// The editor's only NodeDelegateModel. Forwards every question QtNodes asks
// about a node to the NodeType it shares with the engine.
class NodeAdaptor : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    explicit NodeAdaptor(const loom::NodeType& nodeType);

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

    // Everything except id and position, which stay with QtNodes.
    void setInstance(const loom::NodeInstance& instance);

    // Pin identity is the name; the index is QtNodes' own bookkeeping.
    std::string        pinName  (QtNodes::PortType portType, QtNodes::PortIndex index) const;
    QtNodes::PortIndex portIndex(QtNodes::PortType portType, const std::string& pin) const;

    // The author's control over a variadic node's pin count.
    void setExtraPins(int count);

public Q_SLOTS:
    void inputConnectionCreated(const QtNodes::ConnectionId& connection) override;
    void inputConnectionDeleted(const QtNodes::ConnectionId& connection) override;

private:
    const loom::PinSpec* pinAt(QtNodes::PortType portType, QtNodes::PortIndex index) const;
    void refreshPins();
    void rebuildEditors();
    QWidget* buildPinButtons();
    void setWired(const std::string& pin, bool on);
    loom::Value pinValue(const loom::PinSpec& pin) const;

    const loom::NodeType& type;
    loom::NodeInstance    data;

    std::vector<loom::PinSpec> inputs;
    std::vector<loom::PinSpec> outputs;
    bool                       constant = false;

    QPointer<QWidget>               body;
    std::map<std::string, QWidget*> editors;
    std::set<std::string>           wired;
};

// One registry entry per node type, grouped by the category the node declares.
std::shared_ptr<QtNodes::NodeDelegateModelRegistry> makeRegistry(const loom::NodeCatalog& catalog);

#endif //LOOM_EDITOR_NODE_ADAPTOR_H
