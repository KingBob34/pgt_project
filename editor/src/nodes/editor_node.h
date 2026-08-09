#ifndef EDITOR_NODE_H
#define EDITOR_NODE_H
#include <memory>
#include <QColor>
#include <QWidget>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeStyle>

// Minimum node widths
inline constexpr int kDefaultNodeWidth = 180;
inline constexpr int kOperatorNodeWidth = 96;

// Pin types. Two pins connect only when their ids match.
inline QtNodes::NodeDataType flowPin()
{
    return QtNodes::NodeDataType{"flow", ""};
}

inline QtNodes::NodeDataType boolPin()
{
    return QtNodes::NodeDataType{"bool", "bool"};
}

inline QtNodes::NodeDataType valuePin()
{
    return QtNodes::NodeDataType{"value", "value"};
}

inline QtNodes::NodeDataType listPin()
{
    return QtNodes::NodeDataType{"list", "list"};
}

class EditorNode : public QtNodes::NodeDelegateModel
{
    Q_OBJECT
public:
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return flowPin();
    }

    // Flow: one wire out, many in. Data: many wires out, one in.
    QtNodes::ConnectionPolicy portConnectionPolicy(QtNodes::PortType portType,
                                                   QtNodes::PortIndex portIndex) const override
    {
        const bool isOutput = portType == QtNodes::PortType::Out;
        if (dataType(portType, portIndex).id == "flow")
        {
            return isOutput ? QtNodes::ConnectionPolicy::One : QtNodes::ConnectionPolicy::Many;
        }
        return isOutput ? QtNodes::ConnectionPolicy::Many : QtNodes::ConnectionPolicy::One;
    }

    // Read by EditorNodeGeometry
    virtual int minimumWidth() const
    {
        return kDefaultNodeWidth;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const) override
    {
        return nullptr;
    }

    QWidget* embeddedWidget() override
    {
        return nullptr;
    }

    bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return true;
    }

protected:
    void setHeaderColor(const QColor& header)
    {
        QtNodes::NodeStyle style = nodeStyle();
        style.GradientColor0 = header;
        style.GradientColor1 = header.darker(200);
        style.GradientColor2 = QColor(64, 64, 64);
        style.GradientColor3 = QColor(58, 58, 58);
        setNodeStyle(style);
    }
};

#endif //EDITOR_NODE_H
