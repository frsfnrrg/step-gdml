#include <QtCore>
#include <QtGui>

#include "window.h"
#include "translate.h"
#include "stdio.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("STEP-GDML");
    QStringList args = app.arguments();

    if (args.length() <= 2) {
        QString ifile;
        if (args.length() == 2) {
            ifile = args[1];
        }
        MainWindow w(ifile);
        w.show();
        return app.exec();
    } else if (args.length() == 3) {
        QString ifile = args[1];
        QString ofile = args[2];
        Handle(TopTools_HSequenceOfShape) shapes = new TopTools_HSequenceOfShape();
        if (!Translator::importSTEP(ifile, shapes)) {
            printf("Import failed. :-(\n");
            return -1;
        }
        if (!Translator::exportGDML(ofile, shapes)) {
            printf("Export failed. :-(\n");
            return -1;
        }
        return 0;
    } else {
        printf("Usage: step-gdml [INPUT_STEP_FILE] [OUTPUT_GDML_FILE]\n");
        return -1;
    }
}
