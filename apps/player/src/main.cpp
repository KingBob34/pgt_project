#include <QApplication>

#include "player_window.h"

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    PlayerWindow window;
    window.show();

    // Lets the story be handed over by "Open with", or by dropping a path on
    // the executable, as well as through the File menu.
    if (argc > 1) window.openStory(QString::fromLocal8Bit(argv[1]));

    return QApplication::exec();
}
