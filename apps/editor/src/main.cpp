#include <QApplication>
#include <QPalette>

#include "editor_window.h"

namespace
{
    QPalette darkPalette()
    {
        const QColor window(45, 45, 48);
        const QColor base(30, 30, 32);
        const QColor text(236, 236, 236);
        const QColor muted(128, 128, 128);
        const QColor accent(0, 120, 215);

        QPalette palette = QApplication::palette();
        palette.setColor(QPalette::Window, window);
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, base);
        palette.setColor(QPalette::AlternateBase, window);
        palette.setColor(QPalette::ToolTipBase, base);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::Button, window);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, accent);

        palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
        palette.setColor(QPalette::Disabled, QPalette::Text, muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);

        return palette;
    }
}

int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    application.setPalette(darkPalette());

    // Filled to whatever screen it opens on; the size set in the constructor is
    // what the window goes back to when it is restored down.
    EditorWindow window;
    window.showMaximized();

    if (argc > 1) window.openStory(QString::fromLocal8Bit(argv[1]));

    return QApplication::exec();
}
