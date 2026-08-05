#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private:
    void createMenus();
    void createToolBar();
    void createFileBrowser();
};

#endif //MAINWINDOW_H
