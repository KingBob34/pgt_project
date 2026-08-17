#include <QApplication>

#include "editor_window.h"

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    // Filled to whatever screen it opens on; the size set in the constructor is
    // what the window goes back to when it is restored down.
    EditorWindow window;
    window.showMaximized();

    if (argc > 1) window.openStory(QString::fromLocal8Bit(argv[1]));

    return QApplication::exec();
}
