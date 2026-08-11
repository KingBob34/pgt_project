#include <QApplication>

#include "editor_window.h"

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    EditorWindow window;
    window.show();

    if (argc > 1) window.openStory(QString::fromLocal8Bit(argv[1]));

    return QApplication::exec();
}
