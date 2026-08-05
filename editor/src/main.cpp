#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.setWindowTitle("Node Editor");
    window.resize(1200, 800);
    window.show();
    return app.exec();
}