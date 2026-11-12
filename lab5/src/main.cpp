#include <QApplication>

#include "main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Lab5 Clipping");

    MainWindow w;
    w.show();

    const QStringList args = QApplication::arguments();
    if (args.size() > 1) {
        w.loadFile(args.at(1));
    } else {
        w.loadFile(":/examples/part1_basic.txt");
    }
    return app.exec();
}
