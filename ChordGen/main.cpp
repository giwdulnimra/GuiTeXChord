#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Grifftabellen-Generator");
    app.setOrganizationName("ChordGen");

    MainWindow win;
    win.show();
    return app.exec();
}
