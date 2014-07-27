#include <QtCore>
#include <QtGui>

#include "window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}
