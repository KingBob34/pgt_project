#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H
#include <QPoint>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/GraphicsView>
#include <unordered_set>
#include <QtNodes/Definitions>

// The canvas
class EditorView : public QtNodes::GraphicsView
{
    Q_OBJECT
public:
    explicit EditorView(QtNodes::BasicGraphicsScene* scene, QWidget* parent = nullptr);
    void protectFromDeletion(QtNodes::NodeId nodeId);

public Q_SLOTS:
    void onDeleteSelectedObjects() override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // Right-button travel allowed before the context menu is suppressed
    static  constexpr int kClickSlack = 4;
    bool panning = false;
    bool panMoved = false;
    QPoint panStartPosition;
    QPoint lastPanPosition;

    std::unordered_set<QtNodes::NodeId> undeletableNodes;
};

#endif //EDITOR_VIEW_H
