#include <QApplication>

#include "main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Lab6 3D Part");

    MainWindow w;
    w.show();
    return app.exec();
}
