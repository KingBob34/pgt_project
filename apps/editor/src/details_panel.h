#ifndef LOOM_EDITOR_DETAILS_PANEL_H
#define LOOM_EDITOR_DETAILS_PANEL_H
#include <map>
#include <string>

#include <QPointer>
#include <QWidget>

class NodeAdaptor;
class QFormLayout;

// Every pin of one node, at a size a paragraph fits in.
class DetailsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsPanel(QWidget* parent = nullptr);

    void setNode(NodeAdaptor* adaptor);

private:
    void clearForm();

    // Follows a value the author typed on the node itself.
    void refresh(const QString& pin);

    QFormLayout*          form = nullptr;
    QPointer<NodeAdaptor> shown;

    std::map<std::string, QWidget*> fields;
};

#endif //LOOM_EDITOR_DETAILS_PANEL_H
